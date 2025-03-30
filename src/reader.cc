#include "src/reader.h"

#include <cctype>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "src/form.h"
#include "src/source_info.h"

namespace dub {

void SourceReader::OnNext(char c) {
  if (c == '\n') {
    prev_column_ = column_;
    line_++;
    column_ = 0;
    cur_line_ = "";
  } else {
    column_++;
    cur_line_ += c;
  }
}

void SourceReader::NewLine() {
  line_++;
  column_ = 0;
  cur_line_ = "";
}

void SourceReader::Enter() {
  auto info = SourceInfo{
    .location = SourceLocation{
      .filename = name_,
      .line_start = line_,
      .column_start = column_,
    },
  };
  info_stack_.push_back(info);
}

void SourceReader::Exit(Form& form) {
  auto info = info_stack_.back();
  info_stack_.pop_back();
  info.location.line_end = line_;
  info.location.column_end = prev_column_;
  infos_.push_back(std::make_unique<SourceInfo>(info));
  form.info = infos_.back().get();
}


absl::StatusOr<std::optional<Form>> Reader::ReadForm() {
  if (stream_.fail() || stream_.bad()) {
    return absl::FailedPreconditionError("stream is in fail/bad state");
  }

  char c = Next();
  while (state_ == Reader::State::kSkip || state_ == Reader::State::kComment) {
	if (state_ == Reader::State::kComment) {
	  ReadComment();
	} 
	c = Next();
  }

  if (source_reader_) source_reader_->Enter();
  absl::StatusOr<Form::Value> value;
  switch (state_) {
    case Reader::State::kEnd:
    case Reader::State::kListEnd:
    case Reader::State::kVectorEnd:
	  return std::nullopt;
    case Reader::State::kString:
      value = ReadString();
      break;
    case Reader::State::kListStart:
      value = ReadList();
      break;
    case Reader::State::kVectorStart:
      value = ReadVector();
      break;
    case Reader::State::kNumberOrSymbol:
      value = ReadLiteralOrSymbol(c);
      break;
    default:
      return absl::InvalidArgumentError("invalid syntax");
  }
  if (!value.ok()) {
    return value.status();
  }
  auto f = Form(std::move(value.value()));
  if (source_reader_) source_reader_->Exit(f);
  return f;
}

absl::StatusOr<List> Reader::ReadAll() {
  MutableList mlist;
  do {
    auto f = ReadForm();
    if (!f.ok()) {
      return f.status();
    }
	if (!f.value().has_value()) break;
	mlist.Append(std::move(f.value().value()));
   } while (!Done());
  return List(mlist);
}

char Reader::Next() {
  char c = GetChar();

  switch (c) {
    case '"':
      state_ = Reader::State::kString;
      break;
    case '(':
      state_ = Reader::State::kListStart;
      break;
    case ')':
      state_ = Reader::State::kListEnd;
      break;
    case '[':
      state_ = Reader::State::kVectorStart;
      break;
    case ']':
      state_ = Reader::State::kVectorEnd;
      break;
    case ';':
      state_ = Reader::State::kComment;
      break;
    default:
      if (stream_.eof()) {
        state_ = Reader::State::kEnd;
      } else if (isspace(c) || c == ',') {
        state_ = Reader::State::kSkip;
      } else {
        state_ = Reader::State::kNumberOrSymbol;
      }
      break;
  }
  return c;
}

const char kTab = 0x09;
const char kNewline = 0x0a;
const char kCarriageReturn = 0x0d;

static std::optional<char> EscapeChar(char c) {
  switch (c) {
    case '\\':
    case '"':
      return c;
    case 'n':
      return kNewline;
    case 't':
      return kTab;
    case 'r':
      return kCarriageReturn;
  }
  return std::optional<char>();
}

constexpr auto max_size = std::numeric_limits<std::streamsize>::max();
void Reader::ReadComment() {
  stream_.ignore(max_size, '\n');
  if (source_reader_) source_reader_->NewLine();
}

absl::StatusOr<std::string> Reader::ReadString() {
  char c = GetChar();
  std::string result;

  while (c != '"') {
    if (c == '\\') {
      char escaped_c = GetChar();
      auto opt_c = EscapeChar(escaped_c);
      if (!opt_c.has_value()) {
        return absl::InvalidArgumentError(std::string("invalid escape sequence: \\") + escaped_c);
      }
      result.push_back(*opt_c);
    } else {
      result.push_back(c);
    }
    c = GetChar();
  }
  return result;
}


absl::StatusOr<Form::Value> Reader::ReadLiteralOrSymbol(char c) {
  std::int64_t num = 0;
  std::int64_t float_div = 0;
  enum states {
    kUnknown,
    kInteger,
    kFloat,
    kSymbol,
  } state = kUnknown;

  std::string str;
  std::int64_t multiplier = 1;

  for (std::size_t i = 0;; i++, c = Next()) {
    if (state_ != Reader::State::kNumberOrSymbol) {
      break;
    }

    str += c;

    if (i == 0) {
      switch (c) {
        case '+':
          multiplier = 1;
          continue;
        case '-':
          multiplier = -1;
          continue;
      }
    }

    if (state == kInteger && c == '.') {
      state = kFloat;
      float_div = 1;
      continue;
    } else if (!isdigit(c)) {
      state = kSymbol;
    } else if (state == kUnknown) {
      state = kInteger;
    }

    if (state != kSymbol) {
      num = (num * 10) + c - 0x30;
      if (state == kFloat) {
        float_div *= 10;
      }
    }
  }

  if (state == kUnknown) {
    state = kSymbol;
  }

  switch (state) {
    case kUnknown:
      return absl::InvalidArgumentError("failed to read");
    case kInteger:
      return num * multiplier;
    case kFloat:
      return static_cast<double>(num) / float_div * multiplier;
    case kSymbol:
      if (str == "nil") {
        return Nil::Get();
      } else if (str == "true") {
        return true;
      } else if (str == "false") {
        return false;
      }
      return &Symbol::Get(str);
  }

  return absl::InvalidArgumentError("failed to read");
}

absl::StatusOr<List> Reader::ReadList() {
  MutableList forms;

  do {
    auto form = ReadForm();
    if (!form.ok()) {
      return form.status();
    } else if (!form.value().has_value()) {
      break;
    }
    forms.Append(form.value().value());
  } while (state_ != Reader::State::kEnd && state_ != Reader::State::kListEnd);

  // Advance to the next state
  Next();

  return List(forms);
}

absl::StatusOr<Vector> Reader::ReadVector() {
  Vector forms;

  do {
    auto form = ReadForm();
    if (!form.ok()) {
      return form.status();
    } else if (!form.value().has_value()) {
      break;
    }
    forms.push_back(form.value().value());
  } while (state_ != Reader::State::kEnd && state_ != Reader::State::kVectorEnd);

  // Advance to the next state
  Next();

  return forms;
}

std::string Reader::StateToString(const Reader::State& state) {
  switch (state) {
    case Reader::State::kEnd:
      return "End";
    case Reader::State::kSkip:
      return "Skip";
    case Reader::State::kNumberOrSymbol:
      return "NumberOrSymbol";
    case Reader::State::kString:
      return "String";
    case Reader::State::kListStart:
      return "ListStart";
    case Reader::State::kListEnd:
      return "ListEnd";
    case Reader::State::kVectorStart:
      return "VectorStart";
    case Reader::State::kVectorEnd:
      return "VectorEnd";
    case Reader::State::kComment:
      return "Comment";
  }
  return "INVALID";
}

std::ostream& operator<<(std::ostream& os, const Reader::State& state) {
  os << Reader::StateToString(state);
  return os;
}
}  // namespace dub
