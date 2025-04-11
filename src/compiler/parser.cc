#include "parser.h"

#include "src/compiler/constant.h"
#include "src/compiler/expr_format.h"
#include "src/compiler/expr_info.h"
#include "src/compiler/expression.h"
#include "src/compiler/module.h"
#include "src/compiler/result.h"
#include "src/form.h"
#include "src/form_format.h"
#include "src/symbol.h"

#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"

#include <algorithm>
#include <llvm/Support/Error.h>
#include <variant>

namespace dub::compiler {

char parser::Error::ID;

absl::StatusOr<Module> Parser::ParseModule(const List &forms) const {
  Module modul;

  auto header = ParseExpr(forms.Head(), &modul);
  if (header.HasError()) {
    return absl::InvalidArgumentError("failed to parse module header");
  }
  modul.SetHeader(std::get<ModuleDecl>(header.Value().kind()));

  for (const auto &form : forms.Tail()) {
    auto expr = ParseExpr(form, &modul);
    if (expr.HasError()) {
      // TODO: We can continue here until the end and report all errors.
      return absl::InvalidArgumentError(
          absl::StrFormat("failed to parse expression: %v, error: %v", form,
                          expr.FormatErrors()));
    }
  }

  return modul;
}

absl::StatusOr<ModuleDecl> Parser::ParseModuleHeader(const Form &form,
                                                     Module *modul) const {
  auto mod_decl = ParseModuleDecl(form.get<List>(), modul);
  if (mod_decl.HasError()) {
    return absl::InvalidArgumentError(
        absl::StrFormat("cannot parse Module header: %v", form));
  }
  return mod_decl.Value();
}

absl::StatusOr<Expression *> Parser::ParseExpression(const Form &form,
                                                     Module *modul) const {
  auto expr = ParseExpr(form, modul);
  // std::cout << "ParseExpression:" << expr << std::endl;
  if (expr.HasError()) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "failed to parse form: %v, error: %v", form, expr.FormatErrors()));
  }
  // Expressions are assigned their ExprInfo here.
  return &modul->AddExpression(std::move(expr.Value()));
}

Result<Expression> Parser::ParseExpr(const Form &form, Module *modul) const {
  auto result = std::visit(Parser::ParseAsExpr(this, form, modul), form.value);
  // std::cout << "ParseExpr:" << result << std::endl;
  return result;
}

Result<If> Parser::ParseIf(const List &list, Module *modul) const {
  auto tail = list.Tail();
  // TODO: update this;
  If if_expr;
  if (tail.IsEmpty()) {
    return Result<decltype(if_expr)>::Unproceedable(
        llvm::createStringError("if expression is empty"));
  }
  auto cond_expr = ParseExpr(tail.Head(), modul);
  if (cond_expr.HasError()) {
    return Result<decltype(if_expr)>::Unproceedable(cond_expr.errors());
  }
  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return Result<decltype(if_expr)>::Unproceedable(
        llvm::createStringError("if expression missing then and else"));
  }
  auto then_expr = ParseExpr(tail.Head(), modul);

  if (then_expr.HasError()) {
    return Result<decltype(if_expr)>::Unproceedable(then_expr.errors());
  }
  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return Result<decltype(if_expr)>::Unproceedable(
        llvm::createStringError("if expression missing else"));
  }
  auto else_expr = ParseExpr(tail.Head(), modul);
  if (else_expr.HasError()) {
    return Result<decltype(if_expr)>::Unproceedable(else_expr.errors());
  }
  tail = tail.Tail();
  return If(std::move(cond_expr.Value()), std::move(then_expr.Value()),
            std::move(else_expr.Value()));
}

// ParseFn expression
// (fn <name> [params] body...)
// (fn foo [x y] (print 42) (+ x y))
Result<Fn> Parser::ParseFn(const List &list, Module *modul) const {
  auto tail = list.Tail();
  Fn fn;
  if (tail.IsEmpty()) {
    return Result<decltype(fn)>::Unproceedable(
        llvm::createStringError("fn form is empty"));
  }
  // TODO: Make this optional
  auto name_sym = tail.Head();
  if (!name_sym.is<const Symbol *>()) {
    return Result<decltype(fn)>::Unproceedable(
        llvm::createStringError("fn form expects fn name as a symbol"));
  }
  fn.SetName(name_sym.get<const Symbol &>());

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return Result<decltype(fn)>::Unproceedable(
        llvm::createStringError("fn missing parameter declaration"));
  }

  auto params = tail.Head();
  if (!params.is<Vector>()) {
    return Result<decltype(fn)>::Unproceedable(
        llvm::createStringError(llvm::formatv(
            "fn parameter list should be a Vector, got: {0}", params)));
  }
  std::vector<const Symbol *> names;
  for (const auto &form : params.get<Vector>()) {
    if (!form.is<const Symbol *>()) {
      return Result<decltype(fn)>::Unproceedable(
          llvm::createStringError(llvm::formatv(
              "fn parameter list should be a Vector of symbols, got: {0}",
              params)));
    }
    names.push_back(form.get<const Symbol *>());
  }
  fn.SetParams(std::move(names));

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return Result<decltype(fn)>::Unproceedable(
        llvm::createStringError("fn missing body"));
  }

  std::vector<Expression> body;
  for (const auto &form : tail) {
    auto expr = ParseExpr(form, modul);
    if (expr.HasError()) {
      fn.SetBody(body);
      return Result<decltype(fn)>::Unproceedable(expr.errors());
    }
    body.push_back(std::move(expr.Value()));
  }
  fn.SetBody(body);
  return fn;
}

Result<ModuleDecl> Parser::ParseModuleDecl(const List &list,
                                           Module *modul) const {
  (void)modul;
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return Result<ModuleDecl>::Unproceedable(
        llvm::createStringError("module form is empty"));
  }
  auto form = tail.Head();
  if (!form.is<const Symbol *>()) {
    return Result<ModuleDecl>::Unproceedable(
        llvm::createStringError("module form expects module name as a symbol"));
  }
  return ModuleDecl(form.get<const Symbol &>());
}

Result<MemberAccess> Parser::ParseMemberAccess(const List &list,
                                               Module *modul) const {
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return Result<MemberAccess>::Unproceedable(
        llvm::createStringError("struct expression is missing"));
  }
  auto struct_expr = ParseExpr(tail.Head(), modul);
  if (struct_expr.HasError()) {
    return Result<MemberAccess>::Unproceedable(struct_expr.errors());
  }

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return Result<MemberAccess>::Unproceedable(
        llvm::createStringError("member symbol is missing"));
  }

  auto form = tail.Head();
  if (!form.is<const Symbol *>()) {
    return Result<MemberAccess>::Unproceedable(llvm::createStringError(
        "member access form expects field name to be a symbol"));
  }
  return MemberAccess(
      std::make_unique<Expression>(std::move(struct_expr.Value())),
      form.get<const Symbol &>());
}

Result<Call> Parser::ParseCall(const List &list, Module *modul) const {
  auto target = ParseExpr(list.Head(), modul);
  std::vector<Expression> args;
  for (const auto &form : list.Tail()) {
    auto expr = ParseExpr(form, modul);
    if (!expr.IsOk()) {
      return Result<Call>::Unproceedable(expr.errors());
    }
    args.push_back(std::move(expr.Value()));
  }
  auto call = Call(std::move(target.Value()), std::move(args));
  // std::cout << "Call:" << call << std::endl;
  return Result<Call>(std::move(call));
  // return call;
}

Result<Return> Parser::ParseReturn(const List &list, Module *modul) const {
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return Return();
  }
  auto value = ParseExpr(tail.Head(), modul);
  if (value.HasError()) {
    return Result<Return>::Unproceedable(value.errors());
  }
  return Return(std::make_unique<Expression>(std::move(value.Value())));
}

Result<VarDef> Parser::ParseVarDef(const List &list, Module *modul) const {
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return Result<VarDef>::Unproceedable(
        llvm::createStringError("var form is empty"));
  }

  auto name = tail.Head();
  if (!name.is<const Symbol *>()) {
    return Result<VarDef>::Unproceedable(
        llvm::createStringError("var form expects var name as a symbol"));
  }

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return Result<VarDef>::Unproceedable(
        llvm::createStringError("var form expects a var expression."));
  }

  auto type_expr = ParseType(tail.Head(), modul);

  if (type_expr.HasError()) {
    // TODO: Set type to Unit?
    return Result<VarDef>::Unproceedable(type_expr.errors());
  }

  // TODO: parse init value

  return VarDef(name.get<const Symbol &>(), type_expr.Value());
}

Result<Set> Parser::ParseVarSet(const List &list, Module *modul) const {
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return Result<Set>::Unproceedable(
        llvm::createStringError("set form is empty"));
  }

  auto place = ParseExpr(tail.Head(), modul);

  if (place.HasError()) {
    return Result<Set>::Unproceedable(place.errors());
  }

  auto assignable_place = place.Value().AsAssignable();
  if (!assignable_place.has_value()) {
    return Result<Set>::Unproceedable(llvm::createStringError(llvm::formatv(
        "set form expects `(set place expression)`, where place is a symbol "
        "name or `(. struct-expr field-name)`; got {0}",
        place.Value())));
  }

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return Result<Set>::Unproceedable(
        llvm::createStringError("set form expects a var expression."));
  }

  auto value = ParseExpr(tail.Head(), modul);
  if (value.HasError()) {
    return Result<Set>::Unproceedable(value.errors());
  }

  return Set(std::move(assignable_place.value()),
             std::make_unique<Expression>(std::move(value.Value())));
}

Result<TypeDef> Parser::ParseTypeDef(const List &list, Module *modul) const {
  TypeDef decl;
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return Result<TypeDef>::Unproceedable(
        llvm::createStringError("type form is empty"));
  }

  auto name = tail.Head();
  if (!name.is<const Symbol *>()) {
    return Result<TypeDef>::Unproceedable(
        llvm::createStringError("type form expects type name as a symbol"));
  }

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return Result<TypeDef>::Unproceedable(
        llvm::createStringError("type form expects a type expression."));
  }

  auto type_expr = ParseType(tail.Head(), modul);

  if (type_expr.HasError()) {
    // TODO: Set type to Unit?
    return Result<TypeDef>::Unproceedable(type_expr.errors());
  }

  return TypeDef(name.get<const Symbol &>(), type_expr.Value());
}

Result<NameDecl> Parser::ParseNameDecl(const List &list, Module *modul) const {
  NameDecl def;
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return Result<NameDecl>::Unproceedable(
        llvm::createStringError("type form is empty"));
  }

  auto name = tail.Head();
  if (!name.is<const Symbol *>()) {
    return Result<NameDecl>::Unproceedable(
        llvm::createStringError("type form expects type name as a symbol"));
  }

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return Result<NameDecl>::Unproceedable(
        llvm::createStringError("type form expects a type expression"));
  }

  auto type_expr = ParseType(tail.Head(), modul);

  if (type_expr.HasError()) {
    // TODO: Set type to Unit?
    return Result<NameDecl>::Unproceedable(type_expr.errors());
  }

  return NameDecl(name.get<const Symbol &>(), type_expr.Value());
}

Result<TypeExpr> Parser::ParseType(const Form &form, Module *modul) const {
  return std::visit(Parser::ParseAsType(this, form, modul), form.value);
}

template <typename T>
Result<Expression> WrapExpressionResult(Result<T> result, const Form &form,
                                        Module *modul) {
  // std::cout << "Wrap:" << result;
  auto expr = modul->MakeExpression(std::move(result.Value()),
                                    std::make_unique<ExprInfo>(*form.info));
  // std::cout << " new expr:" << expr;
  auto new_res = Result<Expression>(std::move(expr));

  // std::cout << " into: " << new_res << std::endl;
  return new_res;
}

Result<Expression>
Parser::ParseAsExpr::operator()(const Constant::Literal &value) const {
  return modul_->MakeExpression(Constant(value),
                                std::make_unique<ExprInfo>(*form_.info));
}

Result<Expression> Parser::ParseAsExpr::operator()(const Symbol *value) const {
  return modul_->MakeExpression(value, std::make_unique<ExprInfo>(*form_.info));
}

Result<Expression> Parser::ParseAsExpr::operator()(const List &list) const {
  const auto &head = list.Head();

  if (head.is<const Symbol *>()) {
    auto &symbol = head.get<const Symbol &>();

    // Special forms
    const auto &specials = special::Symbols();
    if (std::find(specials.begin(), specials.end(), &symbol) !=
        specials.end()) {
      if (symbol == special::kModule) {
        auto mod_decl = parser_->ParseModuleDecl(list, modul_);
        if (mod_decl.HasError()) {
          return Result<Expression>::Unproceedable(mod_decl.errors());
        }
        modul_->SetHeader(mod_decl.Value());
        auto expr =
            modul_->MakeExpression(std::move(mod_decl.Value()),
                                   std::make_unique<ExprInfo>(*form_.info));
        return Result<Expression>(std::move(expr));
      } else if (symbol == special::kIf) {
        return WrapExpressionResult(parser_->ParseIf(list, modul_), form_,
                                    modul_);
      } else if (symbol == special::kFn) {
        return WrapExpressionResult(parser_->ParseFn(list, modul_), form_,
                                    modul_);
      } else if (symbol == special::kReturn) {
        return WrapExpressionResult(parser_->ParseReturn(list, modul_), form_,
                                    modul_);
      } else if (symbol == special::kType) {
        return WrapExpressionResult(parser_->ParseTypeDef(list, modul_), form_,
                                    modul_);
      } else if (symbol == special::kDeclare) {
        return WrapExpressionResult(parser_->ParseNameDecl(list, modul_), form_,
                                    modul_);
      } else if (symbol == special::kVar) {
        return WrapExpressionResult(parser_->ParseVarDef(list, modul_), form_,
                                    modul_);
      } else if (symbol == special::kSet) {
        return WrapExpressionResult(parser_->ParseVarSet(list, modul_), form_,
                                    modul_);
      } else {
        llvm_unreachable(
            llvm::formatv("internal error: unhandled special form '{0}': {1}",
                          symbol, list)
                .str()
                .c_str());
      }
    }

    // member access
    if (symbol.value().at(0) == '.') {
      return WrapExpressionResult(parser_->ParseMemberAccess(list, modul_),
                                  form_, modul_);
    }
  }

  return WrapExpressionResult(parser_->ParseCall(list, modul_), form_, modul_);
}

Result<Expression> Parser::ParseAsExpr::operator()(const Vector &forms) const {
  std::vector<Expression> exprs;
  for (const auto &form : forms) {
    auto expr = parser_->ParseExpr(form, modul_);
    if (expr.HasError()) {
      // TODO: add details
      return Result<Expression>::Unproceedable(expr.errors());
    }
    exprs.push_back(std::move(expr.Value()));
  }
  return modul_->MakeExpression(Array(std::move(exprs)),
                                std::make_unique<ExprInfo>(*form_.info));
}

Result<TypeExpr>
Parser::ParseAsType::operator()(const Form::Value &value) const {
  // Return a Type::Const or Type::Name
  return std::visit(
      [](auto &&v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::int64_t> ||
                      std::is_same_v<T, double>)
          return Result<TypeExpr>(TypeExpr(Constant(v)));
        else if constexpr (std::is_same_v<T, const Symbol *>)
          return Result<TypeExpr>(TypeExpr(*v));
        else
          return Result<TypeExpr>::Unproceedable(
              llvm::createStringError("invalid type expression"));
      },
      value);
}

Result<TypeExpr::Struct> Parser::ParseAsType::ParseStruct(const List &list,
                                                          Module *modul) const {
  auto tail = list.Tail();
  // if (tail.IsEmpty()) {
  //   return parser::Result<TypeExpr::Struct>::Error(TypeExpr::Struct(),
  //   "struct definition is empty");
  // }
  // auto name = tail.Head();
  // if (!name.is<const Symbol*>()) {
  //   return parser::Result<TypeExpr::Struct>::Error(TypeExpr::Struct(),
  //   "TypeExpr::Struct definition expects type name as a symbol");
  // }
  // tail = tail.Tail();
  if (tail.IsEmpty()) {
    return Result<TypeExpr::Struct>::Unproceedable(llvm::createStringError(
        "struct definition expects a vector of field definitions"));
  }

  std::vector<TypeExpr::NameType> fields;
  auto field_defs_vec = tail.Head();
  for (auto field_def : field_defs_vec.get<Vector>()) {
    auto pair = field_def.get<List>();
    auto name = pair.Head();
    auto type = parser_->ParseType(pair.Second(), modul);
    fields.push_back(
        TypeExpr::NameType{name.get<const Symbol *>(), type.Value()});
  }
  return TypeExpr::Struct(fields);
}

Result<TypeExpr> Parser::ParseAsType::operator()(const List &list) const {
  // Return a Type::Construct
  if (list.IsEmpty()) {
    return Result<TypeExpr>::Unproceedable(
        llvm::createStringError("type construct expression is empty"));
  }
  const auto &name = list.Head();
  if (!name.is<const Symbol *>()) {
    return Result<TypeExpr>::Unproceedable(llvm::createStringError(
        "type construct expression expects a symbol as the first element"));
  }

  auto &name_symbol = name.get<const Symbol &>();

  if (name_symbol == Symbol::Get("struct")) {
    auto struct_def = ParseStruct(list, modul_);
    if (struct_def.HasError()) {
      return Result<TypeExpr>::Unproceedable(struct_def.errors());
    }
    return TypeExpr(std::move(struct_def.Value()));
  }

  std::vector<TypeExpr> type_args;
  for (const auto &form : list.Tail()) {
    auto arg = parser_->ParseType(form, modul_);
    if (arg.HasError()) {
      return Result<TypeExpr>::Unproceedable(
          llvm::createStringError("type construct expression args invalid"));
    }
    type_args.push_back(arg.Value());
  }
  return TypeExpr(name_symbol, type_args);
}

Result<TypeExpr> Parser::ParseAsType::operator()(const Vector &vec) const {
  // Returns a type::Tuple
  std::vector<TypeExpr> type_args;
  for (const auto &form : vec) {
    auto arg = parser_->ParseType(form, modul_);
    if (arg.HasError()) {
      return Result<TypeExpr>::Unproceedable(
          llvm::createStringError("type tuple expression element invalid"));
    }
    type_args.push_back(arg.Value());
  }
  return TypeExpr(type_args);
}

} // namespace dub::compiler
