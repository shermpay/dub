#ifndef DUB_SOURCE_INFO_H_
#define DUB_SOURCE_INFO_H_

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>

#include "absl/strings/str_format.h"
#include "absl/container/node_hash_set.h"

#include "src/form.h"

namespace dub {

absl::node_hash_set<std::string>& FilenamesPool();

struct SourceLocation {
  // lifetime is tied to the FilenamesPool static object.
  std::string_view filename;
  std::uint64_t line_start;
  std::uint64_t column_start;
  std::uint64_t line_end;
  std::uint64_t column_end;

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const SourceLocation& pos) noexcept {
    absl::Format(&sink, "%s:(%llu,%llu)-(%llu,%llu)",
                     pos.filename, pos.line_start, pos.column_start,
                 pos.line_end, pos.column_end);
  }

  friend std::ostream& operator<<(std::ostream& os, const SourceLocation location) {
    return os << absl::StreamFormat("%v", location);
  }
};


struct SourceInfo final {
  SourceLocation location;
  // TODO: Update the following
  std::string_view line;
  // List* list;

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const SourceInfo& info) noexcept {
    absl::Format(&sink, "%v:%s", info.location, info.line);
  }

  friend std::ostream& operator<<(std::ostream& os, const SourceInfo info) {
    return os << absl::StreamFormat("%v", info);
  }
};

std::string FormDebugString(const Form& form) noexcept;

}  // namespace dub


#endif /* DUB_SOURCE_INFO_H_ */
