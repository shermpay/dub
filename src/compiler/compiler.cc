#include "src/compiler/compiler.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <system_error>

#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"

#include "src/compiler/expression.h"
#include "src/compiler/module.h"
#include "src/compiler/type_info.h"

namespace dub {


absl::Status Compiler::CompileModule(const List& forms) {
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

  for (const auto& form : forms) {
    auto status = CompileExpression(form, &ctx);
    if (!status.ok()) {
      return status;
    }
  }

  // TODO: Pass the file names in.
  auto mod_name = ctx.parsed_module()->Header().name->value();
  auto bc_file = absl::StrFormat("out/%s.bc", mod_name);
  std::error_code err;
  llvm::raw_fd_ostream bc_ostream(bc_file, err);
  if (err) {
    return absl::InvalidArgumentError(
        absl::StrFormat(
            "failed to open file: %s to write LLVM bitcode; got error: %s",
            bc_file, err.message()));
  }
  llvm::WriteBitcodeToFile(ctx.LlvmModule(), bc_ostream);

  auto asm_file = absl::StrFormat("out/%s.o", mod_name);
  llvm::raw_fd_ostream asm_ostream(asm_file, err);
  if (err) {
    return absl::InvalidArgumentError(
        absl::StrFormat(
            "failed to open file: %s to write LLVM bitcode; got error: %s",
            asm_file, err.message()));
  }
  target_gen_->ConfigureModule(ctx.ll_module());
  return target_gen_->GenerateModule(ctx.ll_module(), llvm::CodeGenFileType::ObjectFile, &asm_ostream);
}

// TODO: Figure out how to share scoped type info between Typer and LlvmGen
//
absl::Status Compiler::CompileExpression(const Form &form,
                                         CompileContext *ctx) {
  auto expr_ptr = parser_.ParseExpression(form, ctx->parsed_module());

  if (!expr_ptr.ok()) {
    return expr_ptr.status();
  }
  auto &expr = *expr_ptr.value();
  auto mod = ctx->parsed_module();
  if (mod == nullptr) {
    return absl::InternalError("compilation context has null Module");
  }
  // TODO: Separate compilation of module header?
  // Once we parse the module header, we can initialize the module.
  if (auto name_symbol = mod->Header().name; name_symbol) {
    ctx->InitLlvmModule();
  }
  logging() << "[PE] " << expr << std::endl;
  logging() << "[EI] " << *mod->ExprGetInfo(expr) << std::endl;

  auto typed_mod = ctx->typed_module();
  if (typed_mod == nullptr) {
    return absl::InternalError("compilation context has null TypedModule");
  }
  // TODO: Move this initialization out.
  auto typer =
      compiler::Typer(type::BuiltinTypes(), type::BuiltinConstructors(),
                      type::BuiltinNames(), typed_mod->type_info());
  auto result = typer.TypeExpression(expr, typed_mod);
  logging() << "[TC] " << result.IsOk() << '\n';
  if (!result.IsOk()) {
    result.LogErrors(logging());
    return absl::InvalidArgumentError("type check failed");
  }
  logging() << "[TI] " << typed_mod->TypeOf(expr) << std::endl;

  auto codegen = compiler::LlvmGen(compiler::Mode::kAot, typed_mod,
                                   ctx->ll_context(), ctx->ll_module());
  auto code = codegen.GenerateExpression(expr);

  if (!code.ok()) {
    return code.status();
  } else if (code.value() == nullptr) {
    logging() << "[CC] NULL" << std::endl;
    // Expression does not generate any code.
    return absl::OkStatus();
  }
  logging() << "[CC] ";
  llvm::raw_os_ostream ll_ostream(logging());
  code.value()->print(ll_ostream, /*IsForDebug=*/true);
  logging() << std::endl;

  return absl::OkStatus();
}

}  // namespace dub
