#ifndef DUB_COMPILER_EXPR_INFO_H_
#define DUB_COMPILER_EXPR_INFO_H_

#include <ostream>

#include "absl/strings/str_format.h"

#include "src/compiler/type.h"
#include "src/source_info.h"

namespace dub::compiler {

// ExprInfo contains information of an expression after parsing.
class ExprInfo final {
 public:
  ExprInfo(SourceInfo& source) : source(&source) {}

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const ExprInfo& info) noexcept {
    absl::Format(&sink, "ExprInfo{source=%v}", *info.source);
  }

  friend std::ostream& operator<<(std::ostream& os, const ExprInfo info) {
    return os << absl::StreamFormat("%v", info);
  }

  SourceInfo* source;
};

}  // namespace dub::compiler

#endif /* DUB_COMPILER_EXPR_INFO_H_ */
