#include "parser.h"

#include <iostream>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "llvm/Support/FormatVariadic.h"

#include "src/compiler/expr_format.h"
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
  if (!arg.ok()) {
    *result_listener << arg.status();
    return false;
  }
  *result_listener << "got form " << arg.value() << ", want form " << expr;
  return *arg.value() == expr;
}

#define ASSERT_OK_EQ(arg, want) ASSERT_THAT(arg, IsOkAndMatch(std::ref(want)))

TEST(ParserTest, ParseLiteral) {
  auto test = Module::WithName(Symbol::Get("testing"));
  Parser parser;
  auto result = parser.ParseExpression(Form(42), &test);
  auto want = Literal(42);
  ASSERT_OK_EQ(result, want);
}

TEST(ParserTest, ParseCall) {
  auto test = Module::WithName(Symbol::Get("testing"));
  Parser parser;
  const auto& form = Form(List(Symbol("foo"), 1, 2));
  auto result = parser.ParseExpression(form, &test);
  auto want = Expression(
      Call(Expression(Symbol("foo")),
           Literal(1),
           Literal(2)));
  ASSERT_OK_EQ(result, want);
}

TEST(ParserTest, ParseIf) {
  auto test = Module::WithName(Symbol::Get("testing"));
  Parser parser;
  auto want = Expression(If(Name("cond"), Name("then"), Name("else")));
  ASSERT_OK_EQ(parser.ParseExpression(
      Form(List(Symbol("if"), Symbol("cond"), Symbol("then"), Symbol("else"))),
      &test), want);
}
