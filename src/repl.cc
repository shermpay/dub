#include "src/compiler/compiler.h"
// for format_provider<Form>
#include "src/form_format.h" // IWYU pragma: keep
#include "src/reader.h"
#include "src/symbol.h"

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>
#include <memory>

namespace dub {
class Repl {
public:
  Repl(dub::Reader &reader, dub::Compiler &compiler, llvm::raw_ostream &outs,
       llvm::raw_ostream &errs)
      : reader_(std::move(reader)), compiler_(std::move(compiler)), outs_(outs),
        errs_(errs) {}

  void Start() {
    auto comp_ctx = CompileContext::WithModuleName(Symbol::Get("user"));
    std::cout << "w REPL" << std::endl;
    while (!reader_.Done()) {
      std::cout << "=> ";
      auto f = reader_.ReadForm();
      if (!f) {
        errs_ << "read failed! Error: " << f.takeError() << '\n';
        continue;
      }
      if (!f->has_value())
        continue;
      auto form = f->value();
      outs_ << llvm::formatv("[RF] {0}\n", form);
      outs_ << llvm::formatv("[RI] {0}\n", *form.info);
      auto err = compiler_.CompileExpression(form, comp_ctx.get());
      if (err) {
        errs_ << "compile failed! Error: " << err << '\n';
        continue;
      }
    }
  }

private:
  dub::Reader reader_;
  dub::Compiler compiler_;
  llvm::raw_ostream &outs_;
  llvm::raw_ostream &errs_;
};

} // namespace dub

int main(int argc, char *argv[]) {
  (void)argc, (void)argv;
  dub::Reader reader(std::cin);
  reader.EnableLocation(dub::SourceReader("*repl*"));
  llvm::raw_fd_ostream outs(fileno(stdout), false);
  llvm::raw_fd_ostream errs(fileno(stderr), false);
  auto compiler = dub::Compiler::MakeDefaultJit(outs);
  if (!compiler) {
    errs << compiler.takeError() << '\n';
    return 1;
  }
  dub::Repl repl(reader, *compiler, outs, errs);
  repl.Start();
  return 0;
}
