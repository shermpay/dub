#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"

#include "src/reader.h"
#include "src/compiler/compiler.h"
#include "src/compiler/target_gen.h"

namespace {

struct Options {
  std::string_view input;
  std::string_view output;
  std::string_view emit_llvm;
};

static absl::Status CompileFile(const Options& opts) {
  std::ifstream in(std::string(opts.input));

  if (!in) {
	return absl::InvalidArgumentError(
		absl::StrFormat("failed to open file: %s", opts.input));
  }

  dub::Reader reader(in);
  reader.EnableLocation(dub::SourceReader(std::string(opts.input)));
  auto forms = reader.ReadAll();
  
  if (!forms.ok()) {
    return forms.status();
  }

  auto compiler = dub::Compiler::MakeDefaultAot(std::cout);
  if (!compiler.ok()) {
	return compiler.status();
  }
  return compiler.value().CompileModule(forms.value());
}

}  // namespace


ABSL_FLAG(std::string, output, "", "output binary file name");
ABSL_FLAG(std::string, emit_llvm, "", "output LLVM IR file name");

int main(int argc, char* argv[]) {
  absl::SetProgramUsageMessage("Usage: dubc INPUT --output OUTPUT");
  auto arg_vec = absl::ParseCommandLine(argc, argv);
  std::cout << "dub compiler\n";

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

  std::cout << "input:  " << arg_vec[1] << '\n';
  std::cout << "output: " << out << std::endl;

  dub::compiler::TargetGen::InitializeAllTargets();

  auto opts = Options{
	.input = arg_vec[1],
	.output = out,
	.emit_llvm = absl::GetFlag(FLAGS_emit_llvm),
  };
  auto status = CompileFile(opts);
  if (!status.ok()) {
    std::cerr << "failed to compile: " << status << std::endl;
    return status.raw_code();
  }

  std::cout << "compilation status: " << status << std::endl;
  return 0;
}
