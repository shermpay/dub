#ifndef DUB_COMPILER_TYPED_MODULE_H_
#define DUB_COMPILER_TYPED_MODULE_H_

#include "src/compiler/module.h"
#include "src/compiler/type.h"
#include "src/compiler/type_info.h"

#include "llvm/ADT/DenseMap.h"

namespace dub {

enum class Builtin {
  kIntAdd,
  kFloatAdd,
};

class BuiltinInfo final {
public:
  BuiltinInfo(Builtin kind) : kind_(kind) {}
  Builtin kind() const { return kind_; }

private:
  Builtin kind_;
};

class TypedModule final {
public:
  explicit TypedModule(const Module &modul)
      : module_(&modul),
        types_(std::vector<Type>(modul.Infos().size(), type::Unit())),
        type_info_(std::make_unique<compiler::TypeInfo>()) {}
  explicit TypedModule(std::vector<Type> types)
      : types_(types), type_info_(std::make_unique<compiler::TypeInfo>()) {}

  void AddExpressionType(const Expression &expr, Type type) {
    auto num_infos = module_->Infos().size();
    if (num_infos > types_.size()) {
      types_.resize(num_infos, type::Unit());
    }
    types_[expr.Id().ToKey()] = type;
  }

  void AddBuiltinInfo(const Expression &expr, BuiltinInfo builtin) {
    builtin_infos_.insert({expr.Id(), builtin});
  }

  Type TypeOf(const Expression &expr) const {
    return types_.at(expr.Id().ToKey());
  }

  const Module *module_ptr() const { return module_; }

  compiler::TypeInfo *type_info() { return type_info_.get(); }

  const compiler::TypeInfo &TypeInfoRef() const { return *type_info_; }

  auto FindBuiltinInfo(const Expression &expr) const {
    return builtin_infos_.find(expr.Id());
  }

  auto BuiltinInfoEnd() const { return builtin_infos_.end(); }

private:
  const Module *module_;
  // Type of expressions
  std::vector<Type> types_;
  std::unique_ptr<compiler::TypeInfo> type_info_;
  llvm::DenseMap<ExprId, BuiltinInfo> builtin_infos_;
};
} // namespace dub

#endif /* DUB_COMPILER_TYPED_MODULE_H_ */
