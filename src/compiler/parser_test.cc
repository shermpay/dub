#include "parser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "llvm/Support/Error.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Testing/Support/Error.h"

// for format_provider<Expression>
#include "src/compiler/expr_format.h" // IWYU pragma: keep
#include "src/compiler/expression.h"
#include "src/compiler/module.h"
#include "src/form.h"
#include "src/symbol.h"
#include "src/testing/data.h"

using dub::Call;
using dub::Expression;
using dub::Form;
using dub::Module;
using dub::Symbol;
using dub::compiler::Parser;
using dub::dsl::List;
using dub::dsl::Symbol;
using dub::dsl::expr::If;
using dub::dsl::expr::Literal;
using dub::dsl::expr::Name;

using llvm::Succeeded;

// MATCHER_P(IsOkAndMatch, expr, absl::StrFormat("parsed result is OK and form
// is %v", expr)) {
//   if (!arg.IsOk()) {
//     *result_listener << arg.Error();
//     return false;
//   }
//   *result_listener << "got form " << arg.Value() << ", want form " << expr;
//   return arg.Value() == expr;
// }

MATCHER_P(
    IsOkAndMatch, expr,
    llvm::formatv("parsed result is OK and form is {0}", expr.get()).str()) {
  auto expected = &arg;
  if (!*const_cast<llvm::Expected<dub::Expression *> *>(expected)) {
    *result_listener << arg.takeError();
    return false;
  }
  *result_listener << "got form " << *arg << ", want form " << expr;
  return **arg == expr;
}

#define ASSERT_OK_EQ(arg, want) ASSERT_THAT(arg, IsOkAndMatch(std::ref(want)))

TEST(ParserTest, ParseLiteral) {
  auto test = Module::WithName(Symbol::Get("testing"));
  Parser parser;
  auto result = parser.ParseExpression(Form(42), &test);
  ASSERT_THAT_EXPECTED(result, Succeeded());
  ASSERT_EQ(**result, Literal(42));
}

TEST(ParserTest, ParseCall) {
  auto test = Module::WithName(Symbol::Get("testing"));
  Parser parser;
  const auto &form = Form(List(Symbol("foo"), 1, 2));
  auto result = parser.ParseExpression(form, &test);
  auto want =
      Expression(Call(Expression(Symbol("foo")), Literal(1), Literal(2)));
  ASSERT_THAT_EXPECTED(result, Succeeded());
  ASSERT_EQ(**result, want);
}

TEST(ParserTest, ParseIf) {
  auto test = Module::WithName(Symbol::Get("testing"));
  Parser parser;
  auto want = Expression(If(Name("cond"), Name("then"), Name("else")));
  auto result = parser.ParseExpression(
      Form(List(Symbol("if"), Symbol("cond"), Symbol("then"), Symbol("else"))),
      &test);
  ASSERT_THAT_EXPECTED(result, Succeeded());
  ASSERT_EQ(**result, want);
}
