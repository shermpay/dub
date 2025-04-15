#ifndef DUB_FORMATTABLE_H
#define DUB_FORMATTABLE_H

#include "llvm/Support/FormatVariadicDetails.h"
#include "llvm/Support/raw_ostream.h"

#include <concepts>
namespace dub {

template <typename S, typename F>
concept Formattable = requires(S stream, const F formattable) {
  { formattable.format(stream) } -> std::same_as<S>;
};

} // namespace dub

namespace llvm {
template <typename T>
  requires dub::Formattable<raw_ostream &, T>
struct format_provider<T> {
  static void format(const T &formattable, raw_ostream &stream,
                     StringRef style) {
    (void)style;
    formattable.format(stream);
  }
};

} // namespace llvm

#endif // DUB_FORMATTABLE_H
