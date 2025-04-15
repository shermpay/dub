#include "src/reader.h"

#include "src/form.h"
#include "src/symbol.h"

#include "llvm/Support/Error.h"
#include "llvm/Testing/Support/Error.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>

using ::dub::Form;
using ::dub::List;
using ::dub::Symbol;
using ::testing::Pointee;
using ::testing::Pointer;
using ::testing::VariantWith;

MATCHER_P(IsOkAndHolds, matcher, "") {
  if (arg) {
    return ExplainMatchResult(matcher, arg.get(), result_listener);
  }
  return false;
}

#define ASSERT_OK(arg) ASSERT_THAT_EXPECTED(arg, llvm::Success())
#define ASSERT_OK_EQ(arg, want) ASSERT_THAT_EXPECTED(arg, llvm::HasValue(want))

static std::string from_u8string(const std::u8string &s) {
  return std::string(s.begin(), s.end());
}

TEST(ReaderTest, ReadFormString) {
  std::istringstream ss(R"("Hello!")");

  dub::Reader reader(ss);

  auto form = reader.ReadForm();

  ASSERT_OK_EQ(form, Form("Hello!"));
}

TEST(ReaderTest, ReadFormStringWithEscapeCharacters) {
  std::istringstream ss(R"("\t\n")");

  dub::Reader reader(ss);

  auto form = reader.ReadForm();

  ASSERT_OK_EQ(form, Form("\t\n"));
}


TEST(ReaderTest, ReadFormStringUtf8) {
  std::istringstream ss(from_u8string(u8"\"你好👋\""));

  dub::Reader reader(ss);

  auto form = reader.ReadForm();

  ASSERT_OK_EQ(form, Form("你好👋"));
}

TEST(ReaderTest, ReadFormInteger) {
  std::istringstream ss("123456 0 -7 -89");

  dub::Reader reader(ss);

  auto form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(123456));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(0));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(-7));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(-89));
}

TEST(ReaderTest, ReadFormFloat) {
  std::istringstream ss("123.456 7. -8.9 0.321 -0.45 -0.1");

  dub::Reader reader(ss);

  auto form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(123.456));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(7.0));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(-8.9));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(0.321));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(-0.45));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(-0.1));
}

TEST(ReaderTest, ReadFormSymbol) {
  std::istringstream ss("foo + - a");

  dub::Reader reader(ss);

  auto form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(dub::Symbol::Get("foo")));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(dub::Symbol::Get("+")));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(dub::Symbol::Get("-")));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(dub::Symbol::Get("a")));
}


TEST(ReaderTest, ReadFormSymbolContainsNumbersAndDots) {
  std::istringstream ss("1a2b 1.a 1.. -. .1 -.1");
  dub::Reader reader(ss);

  auto form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(dub::Symbol::Get("1a2b")));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(dub::Symbol::Get("1.a")));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(dub::Symbol::Get("1..")));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(dub::Symbol::Get("-.")));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(dub::Symbol::Get(".1")));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(dub::Symbol::Get("-.1")));
}


TEST(ReaderTest, ReadFormList) {
  std::istringstream ss("(0 1 foo) ()");
  dub::Reader reader(ss);

  auto form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(List::Make(Form(0), Form(1), Form(Symbol::Get("foo")))));

  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(List::Make()));
}

TEST(ReaderTest, ReadFormNestedList) {
  std::stringstream ss;
  dub::Reader reader(ss);

  ss << "(((42)))";
  auto form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(List::Make(Form((List::Make(Form(List::Make(Form(42)))))))));

  ss.clear();
  ss << "((a) (b))";
  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(List::Make(Form((List::Make(Form(Symbol::Get("a"))))),
                                     Form((List::Make(Form(Symbol::Get("b"))))))));


  ss.clear();
  ss << "(() ())";
  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(List::Make(Form((List::Make())),
                                     Form((List::Make())))));
  ss.clear();
  ss << "(0 (1 2) ((3) (())))";
  form = reader.ReadForm();
  ASSERT_OK_EQ(form, Form(List::Make(Form(0),
                                     Form(List::Make(Form(1), Form(2))),
                                     Form(List::Make(Form(List::Make(Form(3) )), Form(List::Make(Form(List::Make()))))))));
}
