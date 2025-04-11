#include "src/compiler/expression.h"
#include "src/compiler/type_exprs.h"

#include "absl/strings/str_format.h"
#include "absl/types/span.h"

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

const absl::Span<const TypeExpr::NameType> TypeExpr::Struct::fields() const {
  return fields_;
}

TypeExpr::TypeExpr(Struct struct_def) : kind_(struct_def) {}
bool TypeExpr::operator==(const TypeExpr &rhs) const {
  return this->kind_ == rhs.kind_;
}

namespace special {

absl::Span<const Symbol *const> Symbols() {
  static std::vector<const Symbol *> syms{
      &kFn, &kIf, &kReturn, &kModule, &kType, &kDeclare, &kVar, &kSet,
  };
  return syms;
}

} // namespace special

} // namespace dub
