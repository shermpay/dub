#include "form_format.h"

#include "llvm/Support/FormatVariadic.h"

namespace llvm {

void format_provider<dub::Form>::format(const dub::Form &form,
                                        raw_ostream &stream, StringRef style) {
  (void)style;
  form.Match([&stream](auto &&val) { stream << llvm::formatv("{0}", val); });
}

void format_provider<dub::List>::format(const dub::List &list,
                                        raw_ostream &stream, StringRef style) {
  (void)style;
  stream << "(";
  bool first = true;
  for (const auto &form : list) {
    if (first) {
      first = false;
    } else {
      stream << " ";
    }
    stream << llvm::formatv("{0}", form);
  }
  stream << ")";
}


void format_provider<dub::Vector>::format(const dub::Vector &vec, raw_ostream &stream,
                                          StringRef style) {
  (void)style;
  stream << "[";
  bool first = true;
  for (const auto& form : vec) {
    if (first) {
      first = false;
    } else {
      stream << " ";
    }
    stream << llvm::formatv("{0}", form);
  }
  stream << "]";
}

} // namespace llvm
