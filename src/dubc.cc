#include "src/compiler/compiler.h"
#include "src/compiler/target_gen.h"
#include "src/reader.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

// for fileno, stdout, stderr
#include <cstdio> // IWYU pragma: keep
#include <fstream>
#include <llvm/Support/Error.h>
#include <string>
#include <string_view>

namespace {

struct Options {
  std::string_view input;
  std::string_view output;
  std::string_view emit_llvm;
  llvm::raw_ostream &logging;
};

static llvm::Error CompileFile(const Options &opts) {
  std::ifstream in(std::string(opts.input));

  if (!in) {
    return llvm::createStringError(
        llvm::formatv("failed to open file: {0}", opts.input).str());
  }

  dub::Reader reader(in);
  reader.EnableLocation(dub::SourceReader(std::string(opts.input)));
  auto forms = reader.ReadAll();

  if (!forms) {
    return forms.takeError();
  }

  auto compiler = dub::Compiler::MakeDefaultAot(opts.logging);
  if (!compiler) {
    return compiler.takeError();
  }
  return compiler->CompileModule(*forms);
}

} // namespace

ABSL_FLAG(std::string, output, "", "output binary file name");
ABSL_FLAG(std::string, emit_llvm, "", "output LLVM IR file name");

int main(int argc, char *argv[]) {
  llvm::raw_fd_ostream outs(fileno(stdout), false);
  llvm::raw_fd_ostream errs(fileno(stderr), false);

  absl::SetProgramUsageMessage("Usage: dubc INPUT --output OUTPUT");
  auto arg_vec = absl::ParseCommandLine(argc, argv);
  outs << "dub compiler\n";

  if (arg_vec.size() != 2) {
    std::cerr << absl::ProgramUsageMessage() << '\n';
    std::cerr << "INPUT missing" << std::endl;
    return -1;
  }

  auto out = absl::GetFlag(FLAGS_output);
  // if (out.length() == 0) {
  //   std::cerr << absl::ProgramUsageMessage() << '\n';
  //   std::cerr << "--output requires a non-empty file path" << std::endl;
  //   return -1;
  // }

  outs << "input:  " << arg_vec[1] << '\n';
  outs << "output: " << out << '\n';

  dub::compiler::TargetGen::InitializeAllTargets();

  auto opts = Options{
      .input = arg_vec[1],
      .output = out,
      .emit_llvm = absl::GetFlag(FLAGS_emit_llvm),
      .logging = outs,
  };
  auto err = CompileFile(opts);
  if (err) {
    errs << "failed to compile: " << err << '\n';
    return llvm::errorToErrorCode(std::move(err)).value();
  }

  outs << "compilation status: " << err << '\n';
  return 0;
}
