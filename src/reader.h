#ifndef DUB_READER_H_
#define DUB_READER_H_

#include <cstdint>
#include <istream>
#include <llvm/Support/Error.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "src/form.h"
#include "src/source_info.h"

namespace dub {

// SourceReader is a Reader plugin
class SourceReader {
public:
  SourceReader(std::string name) {
    auto &pool = FilenamesPool();
    auto [iter, _] = pool.insert(name);
    name_ = iter->first();
  }

  void OnNext(char c);
  void NewLine();

  void Enter();
  void Exit(Form &f);

private:
  std::string_view name_;
  std::uint64_t line_ = 0;
  std::uint64_t column_ = 0;
  std::uint64_t prev_column_ = 0;
  std::string cur_line_ = "";
  std::vector<SourceInfo> info_stack_;
  std::vector<std::unique_ptr<SourceInfo>> infos_;
};

// Maybe this should be Reader<T>, where is used in Form<T>.
// For the default case it's just Reader<void>, but for other cases it's,
// Reader<SourceInfo>.
class Reader {
public:
  Reader(std::basic_istream<char> &stream) : stream_(stream) {}

  llvm::Expected<std::optional<Form>> ReadForm();
  llvm::Expected<List> ReadAll();

  bool Done() { return stream_.eof(); }

  void EnableLocation(SourceReader reader) {
    source_reader_ = std::make_unique<SourceReader>(std::move(reader));
  }

private:
  enum class State {
    kEnd,
    kSkip,
    kNumberOrSymbol,
    kString,
    kListStart,
    kListEnd,
    kVectorStart,
    kVectorEnd,
    kComment,
  };

  char Next();
  char GetChar() {
    char c = stream_.get();
    if (source_reader_)
      source_reader_->OnNext(c);
    return c;
  }

  // Reads a single line of comment
  void ReadComment();

  llvm::Expected<std::string> ReadString();
  llvm::Expected<Form::Value> ReadLiteralOrSymbol(char first);
  llvm::Expected<List> ReadList();
  llvm::Expected<Vector> ReadVector();

  static std::string StateToString(const State &state);
  friend std::ostream &operator<<(std::ostream &os, const State &state);

  std::basic_istream<char> &stream_;
  // Reader state machine;
  State state_;
  std::unique_ptr<SourceReader> source_reader_;
};

} // namespace dub

#endif /* DUB_READER_H_ */
