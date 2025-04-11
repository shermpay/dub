#include "src/compiler/expr_format.h"

#include "src/compiler/expression.h"

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/FormatVariadicDetails.h"

#include <type_traits>

namespace llvm {

// TODO: Implement using concepts
//  1. Type implements a streamformat concept
//  2. format_provider takes a streamformat concept and forwards the call

void format_provider<dub::TypeExpr>::format(const dub::TypeExpr &type,
                                            raw_ostream &stream,
                                            StringRef style) {
  (void)style;
  type.Match([&stream](const auto &kind) { stream << formatv("{0}", kind); });
}

void format_provider<dub::TypeExpr::Construct>::format(
    const dub::TypeExpr::Construct &type, raw_ostream &stream,
    StringRef style) {
  (void)style;
  stream << formatv("({0}", type.name());
  for (const auto &type : type.types()) {
    stream << formatv(" {0}", type);
  }
  stream << ')';
}

void format_provider<dub::TypeExpr::Tuple>::format(
    const dub::TypeExpr::Tuple &type, raw_ostream &stream, StringRef style) {
  (void)style;
  stream << '[';
  // TODO: change to iterator
  bool first = true;
  for (const auto &type : type.types) {
    if (first) {
      first = false;
      stream << formatv("{0}", type);
    } else {
      stream << formatv(" {0}", type);
    }
  }
  stream << ']';
}

void format_provider<dub::TypeExpr::Struct>::format(
    const dub::TypeExpr::Struct &type, raw_ostream &stream, StringRef style) {
  (void)style;

  stream << "(struct [";

  bool first = true;
  for (const auto &field_def : type.fields()) {
    if (first) {
      first = false;
    } else {
      stream << ' ';
    }
    stream << formatv("({0} {1})", field_def.first, field_def.second);
  }
  stream << ']';
}

void format_provider<dub::TypeDef>::format(const dub::TypeDef &def,
                                           raw_ostream &stream,
                                           StringRef style) {
  (void)style;
  stream << formatv("(type {0} {1})", def.name(), def.type());
}

void format_provider<dub::exprs::ExprBase<>>::format(
    const dub::exprs::ExprBase<> &expr, raw_ostream &stream, StringRef style) {
  (void)style;
  stream << formatv("({0}", expr.Id());
  for (const auto &expr : expr.SubExprs()) {
    stream << formatv(" {0}", expr);
  }
  stream << ')';
}

void format_provider<dub::compiler::Constant>::format(
    const dub::compiler::Constant &cst, raw_ostream &stream, StringRef style) {
  (void)style;
  stream << dub::compiler::Constant::LiteralToString(cst.literal_);
}

void format_provider<dub::VarDef>::format(const dub::VarDef &expr,
                                          raw_ostream &stream,
                                          StringRef style) {
  (void)style;
  stream << formatv("(var {0} {1}", expr.name(), expr.type());
  if (expr.HasInit()) {
    stream << formatv(" {0}", expr.init());
  }
  stream << ')';
}

void format_provider<dub::Fn>::format(const dub::Fn &fn, raw_ostream &stream,
                                      StringRef style) {
  (void)style;
  stream << "(fn ";
  if (fn.name()) {
    stream << formatv("{0} ", *fn.name());
  }
  stream << '[';

  bool first = true;
  for (const auto &sym : fn.params()) {
    if (first) {
      first = false;
    } else {
      stream << ' ';
    }
    stream << formatv("{0}", *sym);
  }
  stream << ']';
  for (const auto &expr : fn.body()) {
    stream << formatv(" {0}", expr);
  }
  stream << ')';
}

void format_provider<dub::Array>::format(const dub::Array &arr,
                                         raw_ostream &stream, StringRef style) {
  (void)style;
  stream << "[";

  bool first = true;
  for (auto &expr : arr.exprs()) {
    if (first) {
      first = false;
    } else {
      stream << ' ';
    }
    stream << formatv("{0}", expr);
  }
  stream << ']';
}

void format_provider<dub::Assignable>::format(const dub::Assignable &expr,
                                              raw_ostream &stream,
                                              StringRef style) {
  (void)style;
  std::visit(
      [&stream](auto &&kind) {
        using T = std::decay_t<decltype(kind)>;
        if constexpr (std::is_pointer_v<T>)
          stream << formatv("{0}", *kind);
        else
          stream << formatv("{0}", kind);
      },
      expr);
}

void format_provider<dub::Set>::format(const dub::Set &expr,
                                       raw_ostream &stream, StringRef style) {
  (void)style;
  stream << formatv("(set {0} {1})", expr.place(), expr.value());
}

void format_provider<dub::MemberAccess>::format(const dub::MemberAccess &expr,
                                                raw_ostream &stream,
                                                StringRef style) {
  (void)style;
  stream << formatv("(. {0} {1})", expr.struct_expr(), expr.member());
}

void format_provider<dub::ModuleDecl>::format(const dub::ModuleDecl &expr,
                                              raw_ostream &stream,
                                              StringRef style) {
  (void)style;
  stream << formatv("(module {0})", *expr.name);
}

void format_provider<dub::NameDecl>::format(const dub::NameDecl &expr,
                                            raw_ostream &stream,
                                            StringRef style) {
  (void)style;
  stream << formatv("(declare {0} {1})", expr.name(), expr.type());
}

void format_provider<dub::Expression>::format(const dub::Expression &expr,
                                              raw_ostream &stream,
                                              StringRef style) {
  expr.Match([&stream, &style](auto &&kind) {
    using T = std::decay_t<decltype(kind)>;
    if constexpr (std::is_base_of_v<dub::exprs::ExprBase<>, T>)
      format_provider<dub::exprs::ExprBase<>>::format(kind, stream, style);
    else
      stream << formatv("{0}", kind);
    // else if constexpr (std::is_same_v<T, dub::Array>)
    //   stream << "array";
    // else if constexpr (std::is_same_v<T, dub::VarDef>)
    //   stream << "vardef";
    // else if constexpr (std::is_same_v<T, dub::Set>)
    //   stream << "set";
    // else if constexpr (std::is_same_v<T, dub::MemberAccess>)
    //   stream << "memberaccess";
    // else if constexpr (std::is_same_v<T, dub::ModuleDecl>)
    //   stream << "moduledecl";
    // else if constexpr (std::is_base_of_v<dub::exprs::ExprBase<>, T>)
    //   stream << "expr_base";
    // else
  });
  // std::visit([&stream](const auto &kind) { stream << "foo"; },
  // expr.kind_);
}
} // namespace llvm
