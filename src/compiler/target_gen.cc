#include "target_gen.h"

#include <memory>
#include <string>

#include "absl/status/statusor.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"

namespace dub::compiler {

void TargetGen::InitializeAllTargets() {
  // TODO: Only initialize current machine targets
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();
}

absl::StatusOr<std::unique_ptr<TargetGen>> TargetGen::CreateDefault() {
  const auto triple_str = llvm::sys::getDefaultTargetTriple();
  const auto triple = llvm::Triple(triple_str);
  std::string err;
  auto target = llvm::TargetRegistry::lookupTarget(triple, err);
  if (!target) {
	return absl::FailedPreconditionError(err);
  }

  auto cpu = "generic";
  auto features = "";
  llvm::TargetOptions opts;
  auto machine = target->createTargetMachine(triple, cpu, features, opts, llvm::Reloc::PIC_);

  return std::make_unique<TargetGen>(triple, std::unique_ptr<llvm::TargetMachine>(machine));
}

void TargetGen::ConfigureModule(llvm::Module* modul) const {
  modul->setDataLayout(machine_->createDataLayout());
  modul->setTargetTriple(triple_);
}

absl::Status TargetGen::GenerateModule(llvm::Module* modul,
									   llvm::CodeGenFileType file_type,
									   llvm::raw_fd_ostream* stream) const {
  llvm::legacy::PassManager pass;

  if (machine_->addPassesToEmitFile(pass, *stream, nullptr, file_type)) {
	return absl::InvalidArgumentError("TargetMachine can't emit a file of this type");
  }
  pass.run(*modul);
  stream->flush();
  return absl::OkStatus();
}

}  // namespace dub::compiler
