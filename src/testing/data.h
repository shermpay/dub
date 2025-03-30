#ifndef DUB_TESTING_DATA_H_
#define DUB_TESTING_DATA_H_

#include "src/form.h"
#include "src/compiler/expression.h"

namespace dub::dsl {

template <typename... Args>
dub::List List(Args... args) {
  dub::MutableList mlist;
  (mlist.Append(dub::Form(args)), ...);
  return dub::List(mlist);
}

static const dub::Symbol* Symbol(const std::string& s) {
  return &dub::Symbol::Get(s);
}

// Contains functions for creating Expression objects.
namespace expr {

using dub::compiler::Constant;
using dub::Expression;

static Expression Literal(const Constant::Literal& value) {
  return Expression(Constant(value));
}

static Expression Name(std::string name) {
  return Expression(Symbol(name));
}

static Expression If(Expression cond,
                     Expression then,
                     Expression els) {
  return Expression(dub::If(
      std::move(cond),
      std::move(then),
      std::move(els)));
}

}  // namespace expr


}  // namespace dub::testing

#endif /* DUB_TESTING_DATA_H_ */
