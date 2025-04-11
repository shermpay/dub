#include "semantic.h"

#include <iostream>
#include <memory>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"

#include "src/compiler/constant.h"
#include "src/compiler/expression.h"
#include "src/compiler/module.h"
#include "src/compiler/result.h"
#include "src/compiler/type.h"
#include "src/compiler/type_info.h"
#include "src/compiler/typed_module.h"

namespace dub::compiler {

char semantic::TypeMismatchError::ID;
char semantic::TypeNameUndefinedError::ID;

semantic::TypeMismatchError
MakeTypeMismatchError(Type want, const Expression &expr, Type got) {
  return semantic::TypeMismatchError(want, &expr, got);
}

semantic::TypeNameUndefinedError
MakeTypeNameUndefinedError(const Symbol &name, const Expression &expr) {
  return semantic::TypeNameUndefinedError(name, &expr);
}

namespace {

// MakeType is a visitor that interprets type expressions and constructs `Type`
// instances.
struct MakeType final {
  Typer &typer_;

  MakeType(Typer &c) : typer_(c) {}

  absl::StatusOr<Type> operator()(const Constant val) const {
    return Type(Constant(val));
  }

  absl::StatusOr<Type> operator()(const Symbol *name) const {
    // TODO: Resolve the type to a type value?
    auto type = typer_.info().LookupType(*name);
    if (!type.has_value()) {
      return absl::NotFoundError(
          absl::StrFormat("type name '%v' is undefined", name));
    }
    return type.value();
  }

  absl::StatusOr<Type> operator()(const TypeExpr::Construct expr) const {
    auto ctor = typer_.info().LookupConstructor(expr.name());
    if (!ctor.has_value())
      return absl::NotFoundError(
          absl::StrFormat("type constructor '%v' is undefined", expr.name()));
    std::vector<Type> type_args;
    for (const auto &type_expr : expr.types()) {
      auto type_arg = type_expr.Match(*this);
      if (!type_arg.ok()) {
        return type_arg;
      }
      type_args.push_back(type_arg.value());
    }
    return (**ctor)(type_args);
  }

  absl::StatusOr<Type> operator()(const TypeExpr::Tuple expr) const {
    std::vector<Type> types;
    for (const auto &type_expr : expr.types) {
      auto type = type_expr.Match(*this);
      if (!type.ok()) {
        return type.status();
      }
      types.push_back(type.value());
    }
    return Type(type::Tuple<Type>(types));
  }
  absl::StatusOr<Type> operator()(const TypeExpr::Struct expr) const {
    Type::Struct st;
    for (const auto &field_def : expr.fields()) {
      auto field_type = field_def.second.Match(*this);
      if (!field_type.ok()) {
        return field_type.status();
      }
      st.AddField(*field_def.first, field_type.value());
    }
    return Type(st);
  }
};

// Check is a visitor of expressions.
// It verifies the types of the expressions and returns the type of the
// expression.
struct Check final {
  Typer &typer;
  TypedModule &typed_module;
  const Expression &the_expr;
  Check(Typer *t, TypedModule *mod, const Expression *expr)
      : typer(*t), typed_module(*mod), the_expr(*expr) {}

  // TODO: Type and TypeError are the output types of a pass
  // Imagine a pass like this `Pass<Type, TypeError>`
  Result<Type> operator()(const compiler::Constant &ce) {
    if (ce.Is<Nil>()) {
      // TODO: how to type nil?
      // This would require deducting the type or using a type assertion of
      // sorts.
    } else if (ce.Is<bool>()) {
      return type::Bool();
    } else if (ce.Is<std::int64_t>()) {
      return type::I64();
    } else if (ce.Is<std::string>()) {
      return type::PtrOf(type::I8());
    }

    return type::Unit();
  }

  Result<Type> operator()(const TypeDef &decl) {
    auto type = decl.type().Match(MakeType(typer));

    if (!type.ok()) {
      return Result<Type>::Unproceedable(
          dub::StatusError::LlvmError(type.status()));
    }

    // TODO: Handle redefinitions.
    typer.info().AddType(decl.name(), type.value());
    return type::Unit();
  }

  Result<Type> operator()(const NameDecl &def) {
    auto type = def.type().Match(MakeType(typer));

    if (!type.ok()) {
      return Result<Type>::Unproceedable(
          dub::StatusError::LlvmError(type.status()));
    }

    // TODO: Handle redefinitions.
    typer.info().AddName(def.name(), type.value());
    return type::Unit();
  }

  Result<Type> operator()(const Call &expr) {
    std::cout << "Call: " << expr << std::endl;
    // TODO: Support any function object
    const auto callee = expr.target().Get<const Symbol *>();

    auto callee_type = typer.info().LookupName(*callee);
    if (!callee_type.has_value()) {
      return Result<Type>::Unproceedable(
          MakeTypeNameUndefinedError(*callee, the_expr));
    }

    auto fn_type = type::Fn::Get(callee_type.value());
    if (!fn_type.ok()) {
      // TODO: Invalid Fn type, this should be caught during creation.
    }
    auto param_types = fn_type->ParamTypes();
    for (size_t i = 0; i < expr.args().size(); ++i) {
      auto arg_type = typer.TypeExpression(expr.args()[i], &typed_module);
      if (!arg_type.IsOk()) {
        // TODO: Wrap error with current context;
        // TODO: proceed assuming correct type.
        return arg_type;
      }
      if (arg_type.Value() != param_types.types()[i]) {
        return Result<Type>::Unproceedable(MakeTypeMismatchError(
            param_types.types()[i], the_expr, arg_type.Value()));
      }
    }

    std::cout << "Callee: " << *callee << std::endl;
    if (*callee == Symbol::Get("add-i64")) {
      std::cout << "Adding BuiltinInfo to Expression: " << the_expr
                << std::endl;
      // TODO: This won't work for high-order funcs
      const auto &arg_types = param_types.types();
      if (arg_types.size() == 2) {
        if (type::IsIntegerType(arg_types[0]) &&
            type::IsIntegerType(arg_types[1])) {
          typed_module.AddBuiltinInfo(the_expr, BuiltinInfo(Builtin::kIntAdd));
        }
      }
    }

    return fn_type->ReturnType();
  }

  Result<Type> operator()(const Fn &fn) {
    // Check if Fn has type signature.
    auto type = typer.info().LookupName(*fn.name());
    if (!type.has_value()) {
      return Result<Type>::Unproceedable(
          MakeTypeNameUndefinedError(*fn.name(), the_expr));
    }
    for (const auto &expr : fn.body()) {
      auto expr_type = typer.TypeExpression(expr, &typed_module);
      if (!expr_type.IsOk()) {
        return expr_type;
      }
    }
    return type::Unit();
  }

  Result<Type> operator()(const Return &expr) {
    if (expr.HasValue()) {
      auto value_type = typer.TypeExpression(expr.Value(), &typed_module);
      if (!value_type.IsOk()) {
        return value_type;
      }
    }
    return type::Unit();
  }

  Result<Type> operator()(const VarDef &def) {
    auto type = def.type().Match(MakeType(typer));
    if (!type.ok()) {
      return Result<Type>::Unproceedable(StatusError(type.status()));
    }
    typer.info().AddName(def.name(), type.value());

    // TODO: check init
    return type::Unit();
  }

  Result<Type> operator()(const Symbol *name) {
    auto type = typer.info().LookupName(*name);
    if (!type.has_value()) {
      return Result<Type>::Unproceedable(
          MakeTypeNameUndefinedError(*name, the_expr));
    }
    return type.value();
  }

  Result<Type> operator()(const auto &expr) {
    // TODO: Assign type to expression.
    std::cerr << "warning: ignoring type of expression => " << expr
              << std::endl;
    return type::Unit();
  }
};

} // namespace

Typer::Typer(
    std::vector<std::pair<const Symbol *, Type>> builtin_types,
    std::vector<std::pair<const Symbol *, type::Constructor *>> builtin_ctors,
    std::vector<std::pair<const Symbol *, Type>> builtin_names,
    TypeInfo *type_info)
    : info_(type_info) {
  for (const auto &pair : builtin_types) {
    info_->AddType(*pair.first, pair.second);
  }
  for (const auto &pair : builtin_ctors) {
    info_->AddConstructor(*pair.first, pair.second);
  }
  for (const auto &pair : builtin_names) {
    info_->AddName(*pair.first, pair.second);
  }
}

Result<std::unique_ptr<TypedModule>> Typer::TypeModule(const Module &mod) {
  auto typed_mod = std::make_unique<TypedModule>(mod);
  for (const auto &expr : mod.Contents()) {
    auto result = TypeExpression(expr, typed_mod.get());
    if (!result.IsOk()) {
      // TODO: Wrap with current context;
      return typed_mod;
    }
  }
  return typed_mod;
}

Result<Type> Typer::TypeExpression(const Expression &expr,
                                   TypedModule *typed_mod) {
  auto type = expr.Match(Check(this, typed_mod, &expr));
  if (!type.IsOk()) {
    // TODO: Wrap with current context;
    return type;
  }
  typed_mod->AddExpressionType(expr, type.Value());
  return type.Value();
}

} // namespace dub::compiler
