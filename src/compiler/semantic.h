#ifndef DUB_COMPILER_SEMANTIC_H_
#define DUB_COMPILER_SEMANTIC_H_

#include <memory>
#include <optional>
#include <string_view>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"

#include "src/compiler/constant.h"
#include "src/compiler/expression.h"
#include "src/compiler/result.h"
#include "src/compiler/type.h"
#include "src/compiler/type_info.h"
#include "src/compiler/typed_module.h"
#include "src/symbol.h"

namespace dub::compiler {

namespace semantic {

class TypeMismatchError final : public llvm::ErrorInfo<TypeMismatchError> {
public:
  static char ID;
  TypeMismatchError(Type want, const Expression *expr, Type got)
      : want_(want), expr_(expr), got_(got),
        message_(absl::StrFormat(
            "type mismatch for expression %v: expected type is %v, got type %v",
            *expr_, want_, got_)) {}
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
        message_(absl::StrFormat("type name '%v' is undefined (expression: %v)",
                                 name_, *expr_)) {}
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

}  // namespace semantic

semantic::TypeMismatchError
MakeTypeMismatchError(Type want, const Expression &expr, Type got);
semantic::TypeNameUndefinedError
MakeTypeNameUndefinedError(const Symbol &name, const Expression &expr);

// Evaluate type expressions and check types.
class Typer final {
 public:
  Typer(
      std::vector<std::pair<const Symbol*, Type>> builtin_types,
      std::vector<std::pair<const Symbol*, type::Constructor*>> builtin_ctors,
      std::vector<std::pair<const Symbol*, Type>> builtin_names,
      TypeInfo* info);

  TypeInfo& info() noexcept {
    return *info_;
  }

  Result<std::unique_ptr<TypedModule>> TypeModule(const Module&);
  Result<Type> TypeExpression(const Expression&, TypedModule* typed_mod);

 private:
  // TODO: This needs to have Scopes.
  TypeInfo* info_;
};

}  // namespace dub::compiler


#endif /* DUB_COMPILER_SEMANTIC_H_ */
