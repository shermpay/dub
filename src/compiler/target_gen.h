#ifndef DUB_TARGET_GEN_H_
#define DUB_TARGET_GEN_H_

#include "absl/status/statusor.h"
#include "llvm/IR/Module.h"
#include "llvm/Target/TargetMachine.h"

namespace dub::compiler {


class TargetGen final {
 public:
  TargetGen(llvm::Triple triple, std::unique_ptr<llvm::TargetMachine> machine) :
	triple_(triple),
	machine_(std::move(machine))
  {}

  static void InitializeAllTargets();
  static void InitializeCurrentMachineTarget();

  static absl::StatusOr<std::unique_ptr<TargetGen>> CreateDefault();

  void ConfigureModule(llvm::Module* modul) const;
  absl::Status GenerateModule(llvm::Module* modul,
							  llvm::CodeGenFileType file_type,
							  llvm::raw_fd_ostream* stream) const;

 private:
  llvm::Triple triple_;
  std::unique_ptr<llvm::TargetMachine> machine_;
};

}  // namespace dub::compiler



#endif /* DUB_TARGET_GEN_H_ */
