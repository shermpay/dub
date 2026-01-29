#ifndef DUB_COMPILER_SEMANTIC_H_
#define DUB_COMPILER_SEMANTIC_H_

#include "src/compiler/constant.h"
#include "src/compiler/expr_format.h"
#include "src/compiler/expression.h"
#include "src/compiler/result.h"
#include "src/compiler/type.h"
#include "src/compiler/type_info.h"
#include "src/compiler/typed_module.h"
#include "src/errors.h"
#include "src/symbol.h"

#include <memory>
#include <optional>
#include <string_view>

namespace dub::compiler {

namespace semantic {

class TypeMismatchError final : public llvm::ErrorInfo<TypeMismatchError> {
public:
  static char ID;
  TypeMismatchError(Type want, const Expression *expr, Type got)
      : want_(want), expr_(expr), got_(got),
        message_(llvm::formatv("type mismatch for expression {0}: expected "
                               "type is {1}, got type {2}",
                               *expr_, want_, got_)
                     .str()) {}
  ~TypeMismatchError() = default;

  void log(llvm::raw_ostream &os) const noexcept override { os << message_; }

  std::error_code convertToErrorCode() const override {
    return make_error_code(ErrorCode::kTypeCheckerError);
  }

private:
  Type want_;
  const Expression *expr_;
  Type got_;
  std::string message_;
};

class TypeNameUndefinedError final
    : public llvm::ErrorInfo<TypeNameUndefinedError> {
public:
  static char ID;
  TypeNameUndefinedError(const Symbol &name, const Expression *expr)
      : name_(name), expr_(expr),
        message_(
            llvm::formatv("type name '{0}' is undefined (expression: `{1}`)",
                          name_, *expr_)
                .str()) {}
  ~TypeNameUndefinedError() = default;

  void log(llvm::raw_ostream &os) const noexcept override { os << message_; }

  std::error_code convertToErrorCode() const override {
    return make_error_code(ErrorCode::kTypeCheckerError);
  }

private:
  const Symbol &name_;
  const Expression *expr_;
  std::string message_;
};

} // namespace semantic

semantic::TypeMismatchError
MakeTypeMismatchError(Type want, const Expression &expr, Type got);
semantic::TypeNameUndefinedError
MakeTypeNameUndefinedError(const Symbol &name, const Expression &expr);

// Evaluate type expressions and check types.
class Typer final {
public:
  Typer(
      std::vector<std::pair<const Symbol *, Type>> builtin_types,
      std::vector<std::pair<const Symbol *, type::Constructor *>> builtin_ctors,
      std::vector<std::pair<const Symbol *, Type>> builtin_names,
      TypeInfo *info);

  TypeInfo &info() noexcept { return *info_; }

  Result<std::unique_ptr<TypedModule>> TypeModule(const Module &);
  Result<Type> TypeExpression(const Expression &, TypedModule *typed_mod);

private:
  // TODO: This needs to have Scopes.
  TypeInfo *info_;
};

} // namespace dub::compiler

#endif /* DUB_COMPILER_SEMANTIC_H_ */
