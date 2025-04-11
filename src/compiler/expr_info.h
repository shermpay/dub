#ifndef DUB_COMPILER_EXPR_INFO_H_
#define DUB_COMPILER_EXPR_INFO_H_

#include "src/formattable.h"
#include "src/source_info.h"

#include "absl/strings/str_format.h"
#include "llvm/Support/FormatVariadic.h"

#include <ostream>

namespace dub::compiler {

// ExprInfo contains information of an expression after parsing.
class ExprInfo final {
public:
  ExprInfo(SourceInfo &source) : source(&source) {}

  template <typename S>
    requires dub::OutputableStream<S, ExprInfo>
  S &format(S &stream) const {
    return stream
           << llvm::formatv("ExprInfo{{source={0}}}", *this->source).str();
  }

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const ExprInfo &info) noexcept {
    absl::Format(&sink, "ExprInfo{source=%v}", *info.source);
  }

  friend std::ostream &operator<<(std::ostream &os, const ExprInfo info) {
    return info.format(os);
  }

  SourceInfo *source;
};

} // namespace dub::compiler

#endif /* DUB_COMPILER_EXPR_INFO_H_ */
