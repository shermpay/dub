#include "src/compiler/expression.h"

#include "src/compiler/expr_format.h" // IWYU pragma: keep
#include "src/compiler/type_exprs.h"

#include "llvm/Support/FormatVariadic.h"

namespace dub {

TypeExpr TypeUnit() {
  static TypeExpr t(Symbol::Get("unit"));
  return t;
}

TypeExpr::Struct::Struct() : fields_(std::vector<TypeExpr::NameType>()) {}
TypeExpr::Struct::Struct(std::vector<TypeExpr::NameType> fields)
    : fields_(fields) {}

bool TypeExpr::Struct::operator==(const Struct &rhs) const {
  return fields_ == rhs.fields_;
}

const llvm::ArrayRef<TypeExpr::NameType> TypeExpr::Struct::fields() const {
  return fields_;
}

TypeExpr::TypeExpr(Struct struct_def) : kind_(struct_def) {}
bool TypeExpr::operator==(const TypeExpr &rhs) const {
  return this->kind_ == rhs.kind_;
}

namespace exprs {

std::ostream &operator<<(std::ostream &os, const ExprBase<Expression> &expr) {
  os << llvm::formatv("{0}", expr).str();
  return os;
}

std::ostream &operator<<(std::ostream &os, const Call<Expression> &expr) {
  os << llvm::formatv("{0}", expr).str();
  return os;
}

std::ostream &operator<<(std::ostream &os, const Fn<Expression> &fn) {
  os << llvm::formatv("{0}", fn).str();
  return os;
}

std::ostream &operator<<(std::ostream &os, const Array<Expression> &expr) {
  os << llvm::formatv("{0}", expr).str();
  return os;
}

std::ostream &operator<<(std::ostream &os, const VarDef<Expression> &var) {
  os << llvm::formatv("{0}", var).str();
  return os;
}

std::ostream &operator<<(std::ostream &os,
                         const MemberAccess<Expression> &access) {
  os << llvm::formatv("{0}", access).str();
  return os;
}

std::ostream &operator<<(std::ostream &os, const Set<Expression> &expr) {
  os << llvm::formatv("{0}", expr).str();
  return os;
}

} // namespace exprs

std::ostream &operator<<(std::ostream &os, const NameDecl &def) {
  os << llvm::formatv("{0}", def).str();
  return os;
}

std::ostream &operator<<(std::ostream &os, const ModuleDecl &def) {
  os << llvm::formatv("{0}", def).str();
  return os;
}

std::ostream &operator<<(std::ostream &os, const Expression &expression) {
  os << llvm::formatv("{0}", expression).str();
  return os;
}

namespace special {

llvm::ArrayRef<const Symbol *const> Symbols() {
  static std::vector<const Symbol *> syms{
      &kFn, &kIf, &kReturn, &kModule, &kType, &kDeclare, &kVar, &kSet,
  };
  return syms;
}

} // namespace special

} // namespace dub
