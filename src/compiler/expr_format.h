#ifndef DUB_COMPILER_EXPR_FORMAT_H
#define DUB_COMPILER_EXPR_FORMAT_H

#include "src/compiler/constant.h"
#include "src/compiler/expression.h"
#include "src/compiler/type_exprs.h"

#include "llvm/Support/FormatVariadicDetails.h"

namespace llvm {

template <> struct format_provider<dub::TypeExpr> {
  static void format(const dub::TypeExpr &, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::TypeExpr::Construct> {
  static void format(const dub::TypeExpr::Construct &, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::TypeExpr::Tuple> {
  static void format(const dub::TypeExpr::Tuple &, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::TypeExpr::Struct> {
  static void format(const dub::TypeExpr::Struct &, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::TypeDef> {
  static void format(const dub::TypeDef &, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::exprs::ExprBase<>> {
  static void format(const dub::exprs::ExprBase<> &, raw_ostream &stream,
                     StringRef style);
};

// template <typename T>
// std::enable_if_t<
//     std::is_base_of_v<dub::exprs::ExprBase<>, T>> struct format_provider<T> {
//   static void format(const T &, raw_ostream &stream, StringRef style);
// };

template <> struct format_provider<dub::compiler::Constant> {
  static void format(const dub::compiler::Constant &, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::Fn> {
  static void format(const dub::Fn &fn, raw_ostream &stream, StringRef style);
};

template <> struct format_provider<dub::Array> {
  static void format(const dub::Array &arr, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::VarDef> {
  static void format(const dub::VarDef &expr, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::Set> {
  static void format(const dub::Set &expr, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::Assignable> {
  static void format(const dub::Assignable &expr, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::MemberAccess> {
  static void format(const dub::MemberAccess &expr, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::ModuleDecl> {
  static void format(const dub::ModuleDecl &expr, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::NameDecl> {
  static void format(const dub::NameDecl &expr, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::Expression> {
  static void format(const dub::Expression &, raw_ostream &stream,
                     StringRef style);
};

} // namespace llvm

#endif // DUB_COMPILER_EXPR_FORMAT_H
