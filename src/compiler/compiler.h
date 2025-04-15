#ifndef DUB_COMPILER_COMPILER_H_
#define DUB_COMPILER_COMPILER_H_

#include "src/compiler/llvm_gen.h"
#include "src/compiler/module.h"
#include "src/compiler/parser.h"
#include "src/compiler/semantic.h"
#include "src/compiler/target_gen.h"
#include "src/compiler/type_info.h"
#include "src/compiler/typed_module.h"
#include "src/form.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>

namespace dub {

// Contains state for a single compilation unit.
class CompileContext final {
public:
  CompileContext()
      : module_(std::make_unique<Module>()),
        typed_module_(std::make_unique<TypedModule>(*module_)),
        ll_context_(std::make_unique<llvm::LLVMContext>()) {}
  CompileContext(std::unique_ptr<Module> modul,
                 std::unique_ptr<TypedModule> typed_module)
      : module_(std::move(modul)), typed_module_(std::move(typed_module)),
        ll_context_(std::make_unique<llvm::LLVMContext>()),
        ll_module_(std::make_unique<llvm::Module>(
            module_->Header().name->value(), *ll_context_)) {}

  static std::unique_ptr<CompileContext> WithModuleName(const Symbol &name) {
    auto mod = std::make_unique<Module>(Module::WithName(name));
    return std::make_unique<CompileContext>(
        std::move(mod), std::make_unique<TypedModule>(*mod));
  }

  Module *parsed_module() { return module_.get(); }
  TypedModule *typed_module() { return typed_module_.get(); }

  llvm::LLVMContext *ll_context() { return ll_context_.get(); }

  void InitLlvmModule() {
    if (!ll_module_)
      ll_module_ = std::make_unique<llvm::Module>(
          module_->Header().name->value(), *ll_context_);
  }

  llvm::Module *ll_module() { return ll_module_.get(); }

  const llvm::Module &LlvmModule() { return *ll_module_; }

private:
  std::unique_ptr<dub::Module> module_;
  std::unique_ptr<dub::TypedModule> typed_module_;

  std::unique_ptr<llvm::LLVMContext> ll_context_;
  std::unique_ptr<llvm::Module> ll_module_;
  // TODO: This needs to be scoped.
};

class Compiler final {
public:
  Compiler(compiler::Parser parser,
           std::unique_ptr<compiler::TargetGen> target_gen,
           llvm::raw_ostream &logging)
      : parser_(parser), target_gen_(std::move(target_gen)),
        logging_(&logging) {}

  static llvm::Expected<Compiler> MakeDefaultAot(llvm::raw_ostream &logger) {
    auto target_gen = compiler::TargetGen::CreateDefault();
    if (!target_gen) {
      return target_gen.takeError();
    }
    return Compiler(compiler::Parser(), std::move(*target_gen), logger);
  }

  static llvm::Expected<Compiler> MakeDefaultJit(llvm::raw_ostream &logger) {
    return Compiler(compiler::Parser(), nullptr, logger);
  }

  // Compiles a list of forms representing a module.
  llvm::Error CompileModule(const List &forms);

  // Compile a top-level expression
  llvm::Error CompileExpression(const Form &form, CompileContext *ctx);

private:
  llvm::raw_ostream &logging() { return *logging_; }

  compiler::Parser parser_;
  std::unique_ptr<compiler::TargetGen> target_gen_;
  llvm::raw_ostream *logging_;
};

} // namespace dub

#endif /* DUB_COMPILER_COMPILER_H_ */
