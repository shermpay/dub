#include "module.h"

// Needed for format_provider<dub::ModuleDecl>
#include "src/compiler/expr_format.h" // IWYU pragma: keep

#include "llvm/Support/FormatVariadic.h"

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

std::ostream &operator<<(std::ostream &os, const Module &mod) {
  os << llvm::formatv("{0}", mod).str();
  return os;
}

} // namespace dub

namespace llvm {
void format_provider<dub::Module>::format(const dub::Module &mod,
                                          raw_ostream &stream,
                                          StringRef style) {
  (void)style;

  stream << llvm::formatv("{0}\n", mod.Header());

  for (const auto &expr : mod.Contents()) {
    stream << llvm::formatv("{0}\n", expr);
  }
}

} // namespace llvm
