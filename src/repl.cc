#include <iostream>
#include <memory>

#include "src/compiler/compiler.h"
#include "src/reader.h"
#include "src/symbol.h"

namespace dub {
class Repl {
 public:
  Repl(dub::Reader& reader, dub::Compiler& compiler) :
      reader_(std::move(reader)),
      compiler_(std::move(compiler)) {}

  void Start() {
    auto comp_ctx = CompileContext::WithModuleName(Symbol::Get("user"));
    std::cout << "w REPL" << std::endl;
    while(!reader_.Done()) {
      std::cout << "=> ";
      auto f = reader_.ReadForm();
      if (!f.ok()) {
        std::cerr << "read failed! Error: " << f.status() << std::endl;
        continue;
      }
	  if (!f.value().has_value()) continue;
	  auto form = f.value().value();
	  std::cout << "[RF] " << form << std::endl;
      std::cout << "[RI] " << *form.info << std::endl;
      auto status = compiler_.CompileExpression(form, comp_ctx.get());
      if (!status.ok()) {
        std::cerr << "compile failed! Error: " << status << std::endl;
        continue;
      }
    }
  }

 private:
  dub::Reader reader_;
  dub::Compiler compiler_;
};

}  // namespace dub

int main(int args, char* argv[]) {
  dub::Reader reader(std::cin);
  reader.EnableLocation(dub::SourceReader("*repl*"));
  auto compiler = dub::Compiler::MakeDefaultJit(std::cout);
  if (!compiler.ok()) {
	std::cerr << compiler.status() << std::endl;
	return 1;
  }
  dub::Repl repl(reader, compiler.value());
  repl.Start();
  return 0;
}
