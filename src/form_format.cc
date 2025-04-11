#include "src/form_format.h"

#include "src/symbol.h"

#include "llvm/Support/FormatVariadic.h"

namespace llvm {

void format_provider<dub::Form>::format(const dub::Form &form,
                                        raw_ostream &stream, StringRef style) {
  (void)style;
  form.Match([&stream](auto &&val) {
    using T = std::decay_t<decltype(val)>;
    if constexpr (std::is_same_v<T, const dub::Symbol *>)
      stream << llvm::formatv("{0}", *val);
    else
      stream << llvm::formatv("{0}", val);
  });
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
    stream << formatv("{0}", form);
  }
  stream << ")";
}

void format_provider<dub::Vector>::format(const dub::Vector &vec,
                                          raw_ostream &stream,
                                          StringRef style) {
  (void)style;
  stream << "[";
  bool first = true;
  for (const auto &form : vec) {
    if (first) {
      first = false;
    } else {
      stream << " ";
    }
    stream << formatv("{0}", form);
  }
  stream << "]";
}

} // namespace llvm
