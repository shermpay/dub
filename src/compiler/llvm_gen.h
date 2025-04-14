#ifndef DUB_COMPILER_LLVM_GEN_H_
#define DUB_COMPILER_LLVM_GEN_H_

#include <memory>
#include <optional>

#include "absl/status/status.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "src/compiler/expression.h"
#include "src/compiler/semantic.h"
#include "src/compiler/type.h"
#include "src/compiler/typed_module.h"

namespace dub::compiler {

enum class Mode {
  kAot,
  kJit,
};

class LlvmGen final {
public:
  LlvmGen(Mode mode, const TypedModule *typed_module,
          llvm::LLVMContext *context, llvm::Module *ll_module)
      : context_(context), ll_module_(ll_module),
        builder_(std::make_unique<llvm::IRBuilder<>>(*context_)), mode_(mode),
        module_(typed_module->module_ptr()), typed_module_(typed_module) {}

  llvm::Type *GenerateType(Type type);

  void GenerateModule(const Module &mod);

  llvm::Expected<llvm::Value *> GenerateExpression(const Expression &expr);

  llvm::Error WriteBitcode(llvm::raw_fd_stream *stream);

  friend struct ExprGen;

private:
  void AddLocal(const Symbol &, llvm::AllocaInst *);
  std::optional<llvm::AllocaInst *> LookupLocal(const Symbol &);

  llvm::Expected<llvm::Value *>
  BuiltinFuncGen(BuiltinInfo info, const Symbol &name,
                 const llvm::ArrayRef<Expression> args);

  std::optional<Type> TypeOf(Expression &expr);

  llvm::LLVMContext *context_;
  llvm::Module *ll_module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
  llvm::DenseMap<const Symbol *, llvm::AllocaInst *> locals_;

  const Mode mode_;
  const Module *module_;
  const TypedModule *typed_module_;
};

absl::Status WriteBitcode(const llvm::Module &ll_module,
                          llvm::raw_fd_stream *stream);

} // namespace dub::compiler

#endif /* DUB_COMPILER_LLVM_GEN_H_ */

