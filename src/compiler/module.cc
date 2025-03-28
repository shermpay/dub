#include "module.h"

#include <iostream>

namespace dub {

// compiler::ExprInfo& Module::NewExprInfo(compiler::ExprInfo info) {
//   expr_infos_.push_back(std::make_unique<compiler::ExprInfo>(info));
//   return *expr_infos_.back();
// }


Expression Module::MakeExpression(Expression::Kind kind,
                                  std::unique_ptr<compiler::ExprInfo> info) {
  auto id = NextId();
  auto expr = Expression(kind, id);
  expr_infos_.push_back(std::move(info));
  return expr;
}

}  // namespace dub
