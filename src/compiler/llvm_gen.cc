#include "llvm_gen.h"

#include <iostream>
#include <optional>

#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_os_ostream.h"

#include "src/compiler/expression.h"
#include "src/compiler/type.h"
#include "src/compiler/type_info.h"
#include "src/compiler/typed_module.h"

namespace dub::compiler {

namespace {
struct ConvertType final {
  llvm::LLVMContext &context;

  ConvertType(llvm::LLVMContext &context) : context(context) {}

  llvm::Type *Convert(Type type) {
    // std::cout << "Convert: " << type << '\n';
    return type.Match(*this);
  }

  llvm::Type *operator()(type::Basic type) {
    switch (type) {
    case type::Basic::kUnit:
      return llvm::Type::getVoidTy(context);
    case type::Basic::kBool:
      return llvm::Type::getInt1Ty(context);
    case type::Basic::kI8:
      return llvm::Type::getInt8Ty(context);
    case type::Basic::kI16:
      return llvm::Type::getInt16Ty(context);
    case type::Basic::kI32:
      return llvm::Type::getInt32Ty(context);
    case type::Basic::kI64:
      return llvm::Type::getInt64Ty(context);
    case type::Basic::kF32:
      return llvm::Type::getFloatTy(context);
    case type::Basic::kF64:
      return llvm::Type::getDoubleTy(context);
    }
  }
  llvm::Type *operator()(Type::Parameterized type) {
    auto tag = type.name->value();
    if (tag == type::kArrayTag.value()) {
      auto size = type.types[0].Get<Constant>().Get<std::int64_t>();
      auto elem_type = Convert(type.types[1]);
      return llvm::ArrayType::get(elem_type, size);
    } else if (tag == type::kFnTag.value()) {
      const auto &fn_type = type.types;
      auto param_type_struct = Convert(fn_type[0]);
      const auto &param_types =
          static_cast<llvm::StructType *>(param_type_struct)->elements();
      const auto &ret_type = Convert(fn_type[1]);
      return llvm::FunctionType::get(ret_type, param_types, /*isVarArg=*/false);
    } else if (tag == type::kPtrTag.value()) {
      return llvm::PointerType::get(Convert(type.types[0]), 0);
    }
    return nullptr;
  }

  llvm::Type *operator()(Type::Tuple tuple) {
    std::vector<llvm::Type *> elements;
    for (const auto &type : tuple.types()) {
      auto ll_type = Convert(type);
      elements.push_back(ll_type);
    }
    return llvm::StructType::get(context, elements,
                                 /*isPacked=*/false);
  }
  llvm::Type *operator()(Type::Struct st) {
    std::vector<llvm::Type *> elements;
    for (const auto &field : st.fields()) {
      auto ll_type = Convert(field.type);
      elements.push_back(ll_type);
    }
    return llvm::StructType::get(context, elements,
                                 /*isPacked=*/false);
  }

  llvm::Type *operator()(Constant type) { (void)type; return nullptr; }
};

} // namespace

void LlvmGen::AddLocal(const Symbol &name, llvm::AllocaInst *inst) {
  locals_.insert({&name, inst});
}

std::optional<llvm::AllocaInst *> LlvmGen::LookupLocal(const Symbol &name) {
  auto iter = locals_.find(&name);
  if (iter == locals_.end()) {
    return std::nullopt;
  }
  return iter->second;
}

absl::StatusOr<llvm::Value *>
LlvmGen::BuiltinFuncGen(BuiltinInfo info, const Symbol &name,
                        const absl::Span<const Expression> args) {
  (void)name;
  std::cout << "BuiltinFuncGen: " << static_cast<int>(info.kind()) << std::endl;
  switch (info.kind()) {
  case Builtin::kIntAdd: {
    auto left = GenerateExpression(args[0]);
    std::cout << "BuiltinFuncGen: left=" << left.status() << std::endl;
    if (!left.ok()) {
      return left.status();
    }
    auto right = GenerateExpression(args[1]);
    std::cout << "BuiltinFuncGen: right=" << right.status() << std::endl;
    if (!right.ok()) {
      return right.status();
    }
    return builder_->CreateNSWAdd(left.value(), right.value());
  }
  case Builtin::kFloatAdd:
    return absl::UnimplementedError("Codegen for float add");
  }
  return nullptr;
}

struct TypeDefGen final {
  LlvmGen &llgen;
  const TypeDef &type_def;

  TypeDefGen(LlvmGen &gen, const TypeDef &expr) : llgen(gen), type_def(expr) {}

  absl::StatusOr<llvm::Type *> Generate() {
    return type_def.type().Match(*this);
  }

  absl::StatusOr<llvm::Type *> operator()(const auto &type) { (void)type; return nullptr; }
  absl::StatusOr<llvm::Type *> operator()(const Type::Tuple &tuple) {
	(void)tuple;
    return absl::UnimplementedError("typedef tuple");
  }
  absl::StatusOr<llvm::Type *> operator()(const Type::Struct &st) {
	(void)st;
    return absl::UnimplementedError("typedef struct");
  }
};

// Visitor for dub::compiler::Expression
struct ExprGen final {
  LlvmGen &llgen;
  const Expression &the_expr;
  ExprGen(LlvmGen &gen, const Expression &expr) : llgen(gen), the_expr(expr) {}

  absl::StatusOr<llvm::Value *> operator()(const Constant &x) const {
    return x.Match(*this);
  }

  absl::StatusOr<llvm::Value *> operator()(const Nil &x) const {
	(void)x;
    return llvm::ConstantPointerNull::get(
        llvm::PointerType::get(*llgen.context_, /*AddressSpace=*/0));
  }

  absl::StatusOr<llvm::Value *> operator()(bool x) const {
    return llvm::ConstantInt::getSigned(llvm::Type::getInt1Ty(*llgen.context_),
                                        x);
  }

  absl::StatusOr<llvm::Value *> operator()(std::int64_t x) const {
    return llvm::ConstantInt::getSigned(llvm::Type::getInt64Ty(*llgen.context_),
                                        x);
  }

  absl::StatusOr<llvm::Value *> operator()(double x) const {
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*llgen.context_), x);
  }

  absl::StatusOr<llvm::Value *> operator()(std::string x) const {
    auto value = llvm::ConstantDataArray::getString(*llgen.context_, x);
    auto global = new llvm::GlobalVariable(
        value->getType(),
        /*isConstant=*/true, llvm::GlobalValue::InternalLinkage, value, "str");
    llgen.ll_module_->insertGlobalVariable(global);
    return global;
  }

  absl::StatusOr<llvm::Value *> operator()(const Symbol *x) const {
    auto alloca_inst = llgen.LookupLocal(*x);
    // TODO: Semantic analysis to check
    if (!alloca_inst.has_value()) {
      return absl::FailedPreconditionError(
          absl::StrFormat("symbol lookup failed: var '%v' is not defined", x));
    }
    return llgen.builder_->CreateLoad(alloca_inst.value()->getAllocatedType(),
                                      alloca_inst.value(), x->value());
  }

  absl::StatusOr<llvm::Value *>
  operator()(const MemberAccess &x) const {
	(void)x;
    return absl::UnimplementedError("member access is not implemented");
  }

  absl::StatusOr<llvm::Value *> operator()(const Call &expr) const {
    auto &target = expr.target();
    std::cout << "Codegen call: " << target << std::endl;

    if (!target.Is<const Symbol *>()) {
      return absl::InvalidArgumentError(
          "call expression requires callee to be a symbol");
    }

    auto target_symbol = target.Get<const Symbol *>();
    if (target_symbol == nullptr) {
      return absl::FailedPreconditionError("call target is null");
    }

    if (auto pair = llgen.typed_module_->FindBuiltinInfo(the_expr);
        pair != llgen.typed_module_->BuiltinInfoEnd()) {
      auto builtin =
          llgen.BuiltinFuncGen(pair->second, *target_symbol, expr.args());
      if (!builtin.ok()) {
        return builtin.status();
      }
      return builtin.value();
    }

    llvm::Function *callee =
        llgen.ll_module_->getFunction(target_symbol->value());
    if (!callee) {
      return absl::InvalidArgumentError(
          absl::StrFormat("cannot find function: %v", target_symbol));
    }

    if (callee->arg_size() != expr.args().size()) {
      return absl::InvalidArgumentError(absl::StrFormat(
          "invalid number of arguments: %d, %v expects %d args ",
          callee->arg_size(), target_symbol, expr.args().size()));
    }
    std::vector<llvm::Value *> args;
    for (const auto &arg_expr : expr.args()) {
      auto arg = arg_expr.Match(*this);
      if (!arg.ok()) {
        return arg;
      }
      args.push_back(arg.value());
    }
    std::cout << "Creating Call instruction" << std::endl;
    return llgen.builder_->CreateCall(
        callee, args, callee->getReturnType()->isVoidTy() ? "" : "calltmp");
  }
  absl::StatusOr<llvm::Value *> operator()(const If &expr) const {
	(void)expr;
    return absl::UnimplementedError("Codegen If");
  }
  absl::StatusOr<llvm::Value *> operator()(const Fn &fn) const {
    std::cout << "Codegen Fn" << std::endl;
    llvm::Function *ll_func = llgen.ll_module_->getFunction(fn.name()->value());
    auto bb = llvm::BasicBlock::Create(*llgen.context_, "entry", ll_func);
    llgen.builder_->SetInsertPoint(bb);
    std::vector<llvm::Value *> ll_exprs;
    for (const auto &expr : fn.body()) {
      auto ll_value = llgen.GenerateExpression(expr);
      if (!ll_value.ok()) {
        // TODO: return error
      }
      if (ll_value.value() != nullptr) {
        ll_exprs.push_back(ll_value.value());
      }
    }

    return ll_func;
  }

  absl::StatusOr<llvm::Value *>
  operator()(const Return &expr) const {
    // TODO: Handle void.
    // TODO: Handle empty return.
    if (expr.HasValue()) {
      auto value = llgen.GenerateExpression(expr.Value());
      if (!value.ok()) {
        return value.status();
      }
      return llgen.builder_->CreateRet(value.value());
    }
    return absl::UnimplementedError("Codegen Return no value");
  }

  absl::StatusOr<llvm::Value *>
  operator()(const VarDef &expr) const {
    auto type = llgen.typed_module_->TypeInfoRef().LookupName(expr.name());
    if (!type.has_value()) {
      return absl::FailedPreconditionError(
          "type checker did not assign valid type to var name");
    }
    auto lltype = llgen.GenerateType(type.value());
    auto inst = llgen.builder_->CreateAlloca(lltype);
    llgen.AddLocal(expr.name(), inst);
    return inst;
  }

  absl::StatusOr<llvm::Value *> operator()(const Set &expr) const {
    auto place = std::visit(
        AssignableVisitor{
            [this, &expr](const Symbol *name) -> absl::StatusOr<llvm::Value *> {
              auto alloca_inst = llgen.LookupLocal(*name);
              if (!alloca_inst.has_value()) {
                return absl::FailedPreconditionError(absl::StrFormat(
                    "%v failed: var '%v' is not defined", expr, expr.place()));
              }

              return alloca_inst.value();
            },
            [this, &expr](const MemberAccess &access)
                -> absl::StatusOr<llvm::Value *> {
              // %2 = alloca %struct.Point, align 4
              // %3 = getelementptr inbounds %struct.Point, ptr %2, i32 0, i32 0
              // Value * 	CreateStructGEP (Type *Ty, Value *Ptr, unsigned
              // Idx, const Twine &Name="")
              // 1. Lookup type of struct_expr
              const auto type =
                  llgen.typed_module_->TypeOf(access.struct_expr());
              const auto lltype = llgen.GenerateType(type);

              // TODO: Move this to memberacces and add context.
              // MemberAccess should generate different instructions based on if
              // it's a load or stor operation.
              // 2. Generate Value for struct_expr
              auto alloca_inst = llgen.LookupLocal(
                  *access.struct_expr().Get<const Symbol *>());
              if (!alloca_inst.has_value()) {
                return absl::FailedPreconditionError(absl::StrFormat(
                    "%v failed: var '%v' is not defined", expr, expr.place()));
              }
              std::cout << "Struct Value: ";
              llvm::raw_os_ostream ll_ostream(std::cout);
              alloca_inst.value()->print(ll_ostream, true);
              std::cout << std::endl;
              // TODO: lookup field
              return this->llgen.builder_->CreateStructGEP(
                  lltype, alloca_inst.value(), 0);
            },
        },
        expr.place());
    auto llvalue = llgen.GenerateExpression(expr.value());
    if (!llvalue.ok()) {
      return llvalue.status();
    }

    return llgen.builder_->CreateStore(llvalue.value(), place.value());
  }

  absl::StatusOr<llvm::Value *> operator()(const ModuleDecl &expr) const {
	(void)expr;
    return nullptr;
  }

  absl::StatusOr<llvm::Value *> operator()(const TypeDef &def) const {
    TypeDefGen gen(llgen, def);
    auto type = gen.Generate();
    if (!type.ok()) {
      return type.status();
    }
    return nullptr;
  }

  absl::StatusOr<llvm::Value *> operator()(const NameDecl &def) const {
    auto type_opt = llgen.typed_module_->TypeInfoRef().LookupName(def.name());
    if (type_opt.has_value()) {
      auto &type = type_opt.value();
      if (type.IsCallable()) {
        if (!type.Is<Type::Parameterized>()) {
          return nullptr;
        }
        auto func_type =
            static_cast<llvm::FunctionType *>(llgen.GenerateType(type));
        auto func_name = def.name().value();
        return llvm::Function::Create(func_type,
                                      llvm::Function::ExternalLinkage,
                                      func_name, llgen.ll_module_);
      } else if (const auto tuple = type.GetIf<Type::Tuple>()) {
        llgen.GenerateType(type);
      }
    }
    return nullptr;
  }
  absl::StatusOr<llvm::Value *>
  operator()(const Array &expr) const {
	(void)expr;
    return absl::UnimplementedError("Codegen Array");
  }
};

llvm::Type *LlvmGen::GenerateType(Type type) {
  return ConvertType(*context_).Convert(type);
}

void LlvmGen::GenerateModule(const Module &mod) {
  (void)mod;
  // ll_module_ = std::make_unique<llvm::Module>(mod.Header().name->value(),
  // *context_);

  // // Generate types based on type_info_
  // for (auto iter = typed_module_->TypeInfoRef().types_cbegin();
  //      iter != typed_module_->TypeInfoRef().types_cend();
  //      ++iter) {
  //   auto& type = iter->second;
  //   if (type.IsCallable()) {
  //     if (!type.Is<Type::Parameterized>()) {
  //     }
  //     auto func_type = static_cast<llvm::FunctionType*>(GenerateType(type));
  //     auto func_name = type.Get<Type::Parameterized>().name->value();
  //     llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
  //     func_name, ll_module_.get());
  //   }
  // }

  // for (const auto& expr : mod.Contents()) {
  //   auto value = GenerateExpression(expr);
  // }
}

absl::StatusOr<llvm::Value *>
LlvmGen::GenerateExpression(const Expression &expr) {
  return expr.Match(ExprGen(*this, expr));
}

} // namespace dub::compiler
