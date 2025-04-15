#include "src/compiler/compiler.h"

#include <cerrno>
#include <cstring>
#include <system_error>

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#include "src/compiler/expr_info.h"
#include "src/compiler/expression.h"
#include "src/compiler/module.h"
#include "src/formattable.h"

namespace dub {

llvm::Error Compiler::CompileModule(const List &forms) {
  CompileContext ctx;
  // auto mod = parser_.ParseModule(forms);

  // if (!mod.ok()) {
  //   return mod.status();
  // }
  // logging_ << "[PE] " << mod.value() << std::endl;

  // compiler::TypeInfo type_info;
  // auto typer = compiler::Typer(type::BuiltinTypes(),
  //                              type::BuiltinConstructors(),
  //                              type::BuiltinNames(),
  //                              &type_info);

  // auto result = typer->TypeModule(mod.value());
  // logging_ << "[TC] " << result.IsOk() << std::endl;

  // const auto& typed_mod = result.Value();
  // auto codegen = compiler::LlvmGen(typed_mod.get());
  // codegen.GenerateModule(mod.value());

  for (const auto &form : forms) {
    auto err = CompileExpression(form, &ctx);
    if (!err) {
      return err;
    }
  }

  // TODO: Pass the file names in.
  auto mod_name = ctx.parsed_module()->Header().name->value();
  auto bc_file = llvm::formatv("out/{0}.bc", mod_name).str();
  std::error_code err;
  llvm::raw_fd_ostream bc_ostream(bc_file, err);
  if (err) {
    return llvm::createStringError(
        llvm::formatv(
            "failed to open file: {0} to write LLVM bitcode; got error: {1}",
            bc_file, err.message())
            .str());
  }
  llvm::WriteBitcodeToFile(ctx.LlvmModule(), bc_ostream);

  auto asm_file = llvm::formatv("out/{0}.o", mod_name).str();
  llvm::raw_fd_ostream asm_ostream(asm_file, err);
  if (err) {
    return llvm::createStringError(
        llvm::formatv(
            "failed to open file: {0} to write LLVM bitcode; got error: {1}",
            asm_file, err.message())
            .str(),
        std::make_error_code(std::errc::invalid_argument));
  }
  target_gen_->ConfigureModule(ctx.ll_module());
  auto mod = target_gen_->GenerateModule(
      ctx.ll_module(), llvm::CodeGenFileType::ObjectFile, &asm_ostream);
  return mod;
}

// TODO: Figure out how to share scoped type info between Typer and LlvmGen
//
llvm::Error Compiler::CompileExpression(const Form &form, CompileContext *ctx) {
  auto expr_ptr = parser_.ParseExpression(form, ctx->parsed_module());

  if (!expr_ptr) {
    return expr_ptr.takeError();
  }
  auto &expr = *expr_ptr.get();
  auto mod = ctx->parsed_module();
  if (mod == nullptr) {
    return llvm::createStringError("compilation context has null Module");
  }
  // TODO: Separate compilation of module header?
  // Once we parse the module header, we can initialize the module.
  if (auto name_symbol = mod->Header().name; name_symbol) {
    ctx->InitLlvmModule();
  }
  logging() << llvm::formatv("[PE] {0}\n", expr);
  logging() << llvm::formatv("[EI] {0}\n", *mod->ExprGetInfo(expr));

  auto typed_mod = ctx->typed_module();
  if (typed_mod == nullptr) {
    return llvm::createStringError("compilation context has null TypedModule");
  }
  // TODO: Move this initialization out.
  auto typer =
      compiler::Typer(type::BuiltinTypes(), type::BuiltinConstructors(),
                      type::BuiltinNames(), typed_mod->type_info());
  auto result = typer.TypeExpression(expr, typed_mod);
  logging() << "[TC] " << result.IsOk() << '\n';
  if (!result.IsOk()) {
    result.LogErrors(logging());
    return llvm::createStringError(std::errc::invalid_argument,
                                   "type check failed");
  }
  logging() << llvm::formatv("[TI] {0}\n", typed_mod->TypeOf(expr));

  auto codegen = compiler::LlvmGen(compiler::Mode::kAot, typed_mod,
                                   ctx->ll_context(), ctx->ll_module());
  auto code = codegen.GenerateExpression(expr);

  if (!code) {
    return code.takeError();
  } else if (*code == nullptr) {
    logging() << "[CC] NULL\n";
    // Expression does not generate any code.
    return llvm::Error::success();
  }
  logging() << "[CC] ";
  code.get()->print(logging(), /*IsForDebug=*/true);
  logging() << '\n';

  return llvm::Error::success();
}

} // namespace dub
