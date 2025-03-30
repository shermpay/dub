#include "parser.h"

#include <algorithm>
#include <iostream>
#include <variant>

#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"

#include "src/compiler/constant.h"
#include "src/compiler/expression.h"
#include "src/compiler/expr_info.h"
#include "src/compiler/module.h"
#include "src/form.h"
#include "src/symbol.h"

namespace dub::compiler {

absl::StatusOr<Module> Parser::ParseModule(const List& forms) const {
  Module modul;

  auto header = ParseExpr(forms.Head(), &modul);
  if (header.IsError()) {
    return absl::InvalidArgumentError("failed to parse module header");
  }
  modul.SetHeader(std::get<ModuleDecl>(header.Value().kind()));

  for (const auto& form : forms.Tail()) {
    auto expr = ParseExpr(form, &modul);
    if (expr.IsError()) {
      // TODO: We can continue here until the end and report all errors.
      return absl::InvalidArgumentError(
          absl::StrFormat("failed to parse expression: %v, error: %v", form, expr.Error()));
    }
  }

  return modul;
}

absl::StatusOr<ModuleDecl> Parser::ParseModuleHeader(const Form& form, Module* modul) const {
  auto mod_decl = ParseModuleDecl(form.get<List>(), modul);
  if (mod_decl.IsError()) {
    return absl::InvalidArgumentError(
        absl::StrFormat("cannot parse Module header: %v", form));
  }
  return mod_decl.Value();
}

absl::StatusOr<Expression*> Parser::ParseExpression(const Form& form, Module* modul) const {
  auto expr = ParseExpr(form, modul);
  // std::cout << "ParseExpression:" << expr << std::endl;
  if (expr.IsError()) {
    return absl::InvalidArgumentError(
        absl::StrFormat("failed to parse form: %v, error: %v", form, expr.Error()));
  }
  // Expressions are assigned their ExprInfo here.
  return &modul->AddExpression(std::move(expr.Value()));
}

parser::Result<Expression> Parser::ParseExpr(const Form& form, Module* modul) const {
  auto result = std::visit(Parser::ParseAsExpr(this, form, modul), form.value);
  // std::cout << "ParseExpr:" << result << std::endl;
  return result;
}


parser::Result<If> Parser::ParseIf(const List& list, Module* modul) const {
  auto tail = list.Tail();
  // TODO: update this;
  If if_expr;
  if (tail.IsEmpty()) {
    return parser::Result<decltype(if_expr)>::Error(std::move(if_expr), "if expression is empty");
  }
  auto cond_expr = ParseExpr(tail.Head(), modul);
  if (cond_expr.IsError()) {
    return cond_expr.UpdateValue(std::move(if_expr));
  }
  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<decltype(if_expr)>::Error(std::move(if_expr),
                                     "if expression missing then and else");
  }
  auto then_expr = ParseExpr(tail.Head(), modul);

  if (then_expr.IsError()) {
    return then_expr.UpdateValue(std::move(if_expr));
  }
  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<decltype(if_expr)>::Error(std::move(if_expr),
                                     "if expression missing else");
  }
  auto else_expr = ParseExpr(tail.Head(), modul);
  if (else_expr.IsError()) {
    return else_expr.UpdateValue(std::move(if_expr));
  }
  tail = tail.Tail();
  return If(std::move(cond_expr.Value()),
          std::move(then_expr.Value()),
          std::move(else_expr.Value()));
}

// ParseFn expression
// (fn <name> [params] body...)
// (fn foo [x y] (print 42) (+ x y))
parser::Result<Fn> Parser::ParseFn(const List& list, Module* modul) const {
  auto tail = list.Tail();
  Fn fn;
  if (tail.IsEmpty()) {
    return parser::Result<decltype(fn)>::Error(std::move(fn), "fn form is empty");
  }
  // TODO: Make this optional
  auto name_sym = tail.Head();
  if (!name_sym.is<const Symbol*>()) {
    return parser::Result<decltype(fn)>::Error(std::move(fn), "fn form expects fn name as a symbol");
  }
  fn.SetName(name_sym.get<const Symbol&>());

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<decltype(fn)>::Error(std::move(fn), "fn missing parameter declaration");
  }

  auto params = tail.Head();
  if (!params.is<Vector>()) {
    return parser::Result<decltype(fn)>::Errorf(std::move(fn), "fn parameter list should be a Vector, got: %v", params);
  }
  std::vector<const Symbol*> names;
  for (const auto& form : params.get<Vector>()) {
    if (!form.is<const Symbol*>()) {
      return parser::Result<decltype(fn)>::Errorf(
          std::move(fn), "fn parameter list should be a Vector of symbols, got: %v", params);
    }
    names.push_back(form.get<const Symbol*>());
  }
  fn.SetParams(std::move(names));

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<decltype(fn)>::Error(std::move(fn), "fn missing body");
  }

  std::vector<Expression> body;
  for (const auto& form : tail) {
    auto expr = ParseExpr(form, modul);
    if (expr.IsError()) {
      fn.SetBody(body);
      return expr.UpdateValue(std::move(fn));
    }
    body.push_back(std::move(expr.Value()));
  }
  fn.SetBody(body);
  return fn;
}

parser::Result<ModuleDecl> Parser::ParseModuleDecl(const List& list, Module* modul) const {
  (void)modul;
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<ModuleDecl>::Error(ModuleDecl(Symbol::Get("")), "module form is empty");
  }
  auto form = tail.Head();
  if (!form.is<const Symbol*>()) {
    return parser::Result<ModuleDecl>::Error(ModuleDecl(Symbol::Get("")), "module form expects module name as a symbol");
  }
  return ModuleDecl(form.get<const Symbol&>());
}

parser::Result<MemberAccess> Parser::ParseMemberAccess(const List& list, Module* modul) const {
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<MemberAccess>::Error(MemberAccess(), "struct expression is missing");
  }
  auto struct_expr = ParseExpr(tail.Head(), modul);
  if (struct_expr.IsError()) {
	return struct_expr.UpdateValue(MemberAccess());
  }

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<MemberAccess>::Error(MemberAccess(), "member symbol is missing");
  }
  

  auto form = tail.Head();
  if (!form.is<const Symbol*>()) {
    return parser::Result<MemberAccess>::Error(MemberAccess(), "member access form expects field name to be a symbol");
  }
  return MemberAccess(std::make_unique<Expression>(std::move(struct_expr.Value())), form.get<const Symbol&>());
}

parser::Result<Call> Parser::ParseCall(const List& list, Module* modul) const {
  auto target = ParseExpr(list.Head(), modul);
  std::vector<Expression> args;
  for (const auto& form : list.Tail()) {
    auto expr = ParseExpr(form, modul);
    if (!expr.IsOk()) {
      return expr.UpdateValue(Call(std::move(target.Value())));
    }
    args.push_back(std::move(expr.Value()));
  }
  auto call = Call(std::move(target.Value()),
              std::move(args));
  // std::cout << "Call:" << call << std::endl;
  return parser::Result<Call>(std::move(call));
  // return call;
}

parser::Result<Return> Parser::ParseReturn(const List& list, Module* modul) const {
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return Return();
  }
  auto value = ParseExpr(tail.Head(), modul);
  if (value.IsError()) {
    return value.UpdateValue(Return());
  }
  return Return(std::make_unique<Expression>(std::move(value.Value())));
}

parser::Result<VarDef> Parser::ParseVarDef(const List& list, Module* modul) const {
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<VarDef>::Error(VarDef(), "var form is empty");
  }

  auto name = tail.Head();
  if (!name.is<const Symbol*>()) {
    return parser::Result<VarDef>::Error(VarDef(), "var form expects var name as a symbol");
  }

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<VarDef>::Error(VarDef(), "var form expects a var expression.");
  }

  auto type_expr = ParseType(tail.Head(), modul);

  if (type_expr.IsError()) {
    // TODO: Set type to Unit?
    return type_expr.UpdateValue(VarDef());
  }

  // TODO: parse init value

  return VarDef(name.get<const Symbol&>(), type_expr.Value());
}

parser::Result<Set> Parser::ParseVarSet(const List& list, Module* modul) const {
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<Set>::Error(Set(), "set form is empty");
  }

  auto place = ParseExpr(tail.Head(), modul);

  if (place.IsError()) {
	return place.UpdateValue(Set());
  }

  auto assignable_place = place.Value().AsAssignable();
  if (!assignable_place.has_value()) {
    return parser::Result<Set>::Errorf(
        Set(),
        "set form expects `(set place expression)`, where place is a symbol "
          "name or `(. struct-expr field-name)`; got %v",
		place.Value());
  }

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<Set>::Error(Set(), "set form expects a var expression.");
  }

  auto value = ParseExpr(tail.Head(), modul);
  if (value.IsError()) {
	return value.UpdateValue(Set());
  }

  return Set(std::move(assignable_place.value()), std::make_unique<Expression>(std::move(value.Value())));
}

parser::Result<TypeDef> Parser::ParseTypeDef(const List& list, Module* modul) const {
  TypeDef decl;
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<TypeDef>::Error(decl, "type form is empty");
  }

  auto name = tail.Head();
  if (!name.is<const Symbol*>()) {
    return parser::Result<TypeDef>::Error(decl, "type form expects type name as a symbol");
  }

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<TypeDef>::Error(decl, "type form expects a type expression.");
  }

  auto type_expr = ParseType(tail.Head(), modul);

  if (type_expr.IsError()) {
    // TODO: Set type to Unit?
    return type_expr.UpdateValue(std::move(decl));
  }

  return TypeDef(name.get<const Symbol&>(), type_expr.Value());
}

parser::Result<NameDecl> Parser::ParseNameDecl(const List& list, Module* modul) const {
  NameDecl def;
  auto tail = list.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<NameDecl>::Error(def, "type form is empty");
  }

  auto name = tail.Head();
  if (!name.is<const Symbol*>()) {
    return parser::Result<NameDecl>::Error(def, "type form expects type name as a symbol");
  }

  tail = tail.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<NameDecl>::Error(def, "type form expects a type expression.");
  }

  auto type_expr = ParseType(tail.Head(), modul);

  if (type_expr.IsError()) {
    // TODO: Set type to Unit?
    return type_expr.UpdateValue(std::move(def));
  }

  return NameDecl(name.get<const Symbol&>(), type_expr.Value());
}

parser::Result<TypeExpr> Parser::ParseType(const Form& form, Module* modul) const {
  return std::visit(Parser::ParseAsType(this, form, modul), form.value);
}

template <typename T>
parser::Result<Expression> WrapExpressionResult(parser::Result<T> result,
                                                const Form& form,
                                                Module* modul) {
  // std::cout << "Wrap:" << result;
  auto expr = modul->MakeExpression(std::move(result.Value()),
                                    std::make_unique<ExprInfo>(*form.info));
  // std::cout << " new expr:" << expr;
  auto new_res = result.UpdateValue(std::move(expr));

  // std::cout << " into: " << new_res << std::endl;
  return new_res;
}

parser::Result<Expression> Parser::ParseAsExpr::operator()(const Constant::Literal& value) const {
  return modul_->MakeExpression(Constant(value),
                                  std::make_unique<ExprInfo>(*form_.info));
}

parser::Result<Expression> Parser::ParseAsExpr::operator()(const Symbol* value) const {
  return modul_->MakeExpression(value,
                                  std::make_unique<ExprInfo>(*form_.info));
}


parser::Result<Expression> Parser::ParseAsExpr::operator()(const List& list) const {
  const auto& head = list.Head();

  if (head.is<const Symbol*>()) {
    auto& symbol = head.get<const Symbol&>();

	// Special forms
    const auto& specials = special::Symbols();
    if (std::find(specials.begin(), specials.end(), &symbol) != specials.end()) {
      if (symbol == special::kModule) {
        auto mod_decl =parser_->ParseModuleDecl(list, modul_);
        if (mod_decl.IsError()) {
          return mod_decl.UpdateValue(Expression(mod_decl.Value()));
        }
        modul_->SetHeader(mod_decl.Value());
        auto expr = modul_->MakeExpression(std::move(mod_decl.Value()),
                                           std::make_unique<ExprInfo>(*form_.info));
        return mod_decl.UpdateValue(std::move(expr));
      } else if (symbol == special::kIf) {
        return WrapExpressionResult(parser_->ParseIf(list, modul_),
                                    form_,
                                    modul_);
      } else if (symbol == special::kFn) {
        return WrapExpressionResult(parser_->ParseFn(list, modul_),
                                    form_,
                                    modul_);
      } else if (symbol == special::kReturn) {
        return WrapExpressionResult(parser_->ParseReturn(list, modul_),
                                    form_,
                                    modul_);
      } else if (symbol == special::kType) {
        return WrapExpressionResult(parser_->ParseTypeDef(list, modul_),
                                    form_,
                                    modul_);
      } else if (symbol == special::kDeclare) {
        return WrapExpressionResult(parser_->ParseNameDecl(list, modul_),
                                    form_,
                                    modul_);
	  } else if (symbol == special::kVar) {
		return WrapExpressionResult(parser_->ParseVarDef(list, modul_),
									form_,
									modul_);
	  } else if (symbol == special::kSet) {
		return WrapExpressionResult(parser_->ParseVarSet(list, modul_),
									form_,
									modul_);
      } else {
        return parser::Result<Expression>::Errorf(
            Expression(),
            "internal error: unhandled special form '%v': %v", symbol, list);
      }
    }

	// member access
    if (symbol.value().at(0) == '.') {
      return WrapExpressionResult(parser_->ParseMemberAccess(list, modul_), form_, modul_);
    }
  }


  return WrapExpressionResult(parser_->ParseCall(list, modul_), form_, modul_);
}

parser::Result<Expression> Parser::ParseAsExpr::operator()(const Vector& forms) const {
  std::vector<Expression> exprs;
  for (const auto& form : forms) {
    auto expr = parser_->ParseExpr(form, modul_);
    if (expr.IsError()) {
      return expr.UpdateValue(modul_->MakeExpression(
          Array(std::move(exprs)),
          std::make_unique<ExprInfo>(*form.info)));
    }
    exprs.push_back(std::move(expr.Value()));
  }
  return modul_->MakeExpression(
      Array(std::move(exprs)),
      std::make_unique<ExprInfo>(*form_.info));
}


parser::Result<TypeExpr> Parser::ParseAsType::operator()(const Form::Value& value) const {
  // Return a Type::Const or Type::Name
  return std::visit([](auto&& v){
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, std::int64_t> || std::is_same_v<T, double>)
        return parser::Result<TypeExpr>(TypeExpr(Constant(v)));
    else if constexpr (std::is_same_v<T, const Symbol*>)
        return parser::Result<TypeExpr>(TypeExpr(*v));
    else
      return parser::Result<TypeExpr>::Error(
          TypeUnit(),"invalid type expression");
  }, value);
}

parser::Result<TypeExpr::Struct> Parser::ParseAsType::ParseStruct(const List& list, Module* modul) const {
  auto tail = list.Tail(); 
  // if (tail.IsEmpty()) {
  //   return parser::Result<TypeExpr::Struct>::Error(TypeExpr::Struct(), "struct definition is empty");
  // }
  // auto name = tail.Head();
  // if (!name.is<const Symbol*>()) {
  //   return parser::Result<TypeExpr::Struct>::Error(TypeExpr::Struct(), "TypeExpr::Struct definition expects type name as a symbol");
  // }
  // tail = tail.Tail();
  if (tail.IsEmpty()) {
    return parser::Result<TypeExpr::Struct>::Error(TypeExpr::Struct(), "struct definition expects a vector of field definitions");
  }

  std::vector<TypeExpr::NameType> fields;
  auto field_defs_vec = tail.Head();
  for (auto field_def : field_defs_vec.get<Vector>()) {
	auto pair = field_def.get<List>();
	auto name = pair.Head();
	auto type = parser_->ParseType(pair.Second(), modul);
	fields.push_back(TypeExpr::NameType{name.get<const Symbol*>(), type.Value()});
  }
  return TypeExpr::Struct(fields);
}

parser::Result<TypeExpr> Parser::ParseAsType::operator()(const List& list) const {
  // Return a Type::Construct
  if (list.IsEmpty()) {
    return parser::Result<TypeExpr>::Error(TypeUnit(), "type construct expression is empty");
  }
  const auto& name = list.Head();
  if (!name.is<const Symbol*>()) {
    return parser::Result<TypeExpr>::Error(TypeUnit(), "type construct expression expects a symbol as the first element");
  }

  auto& name_symbol = name.get<const Symbol&>();

  if (name_symbol == Symbol::Get("struct")) {
	auto struct_def = ParseStruct(list, modul_);
	if (struct_def.IsError()) {
	  return struct_def.UpdateValue(TypeUnit());
	}
	return TypeExpr(std::move(struct_def.Value()));
  }

  std::vector<TypeExpr> type_args;
  for (const auto& form : list.Tail()) {
    auto arg = parser_->ParseType(form, modul_);
    if (arg.IsError()) {
      return parser::Result<TypeExpr>::Error(
          TypeExpr(name_symbol, type_args), "type construct expression args invalid");
    }
    type_args.push_back(arg.Value());
  }
  return TypeExpr(name_symbol, type_args);
}

parser::Result<TypeExpr> Parser::ParseAsType::operator()(const Vector& vec) const {
  // Returns a type::Tuple
  std::vector<TypeExpr> type_args;
  for (const auto& form : vec) {
    auto arg = parser_->ParseType(form, modul_);
    if (arg.IsError()) {
      return parser::Result<TypeExpr>::Error(
          TypeExpr(type_args), "type tuple expression element invalid");
    }
    type_args.push_back(arg.Value());
  }
  return TypeExpr(type_args);
}

}  // namespace dub::compiler
