#ifndef DUB_SOURCE_INFO_H_
#define DUB_SOURCE_INFO_H_

#include "absl/strings/str_format.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/FormatVariadicDetails.h"

#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>

namespace dub {

llvm::StringSet<> &FilenamesPool();

struct SourceLocation {
  // lifetime is tied to the FilenamesPool static object.
  std::string_view filename;
  std::uint64_t line_start;
  std::uint64_t column_start;
  std::uint64_t line_end;
  std::uint64_t column_end;

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const SourceLocation &pos) noexcept {
    absl::Format(&sink, "%s:(%llu,%llu)-(%llu,%llu)", pos.filename,
                 pos.line_start, pos.column_start, pos.line_end,
                 pos.column_end);
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const SourceLocation location) {
    return os << absl::StreamFormat("%v", location);
  }
};

struct SourceInfo final {
  SourceLocation location;
  // TODO: Update the following
  std::string_view line;
  // List* list;

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const SourceInfo &info) noexcept {
    absl::Format(&sink, "%v:%s", info.location, info.line);
  }

  friend std::ostream &operator<<(std::ostream &os, const SourceInfo info) {
    return os << absl::StreamFormat("%v", info);
  }
};

struct Form;
std::string FormDebugString(const Form &form) noexcept;

} // namespace dub

namespace llvm {

template <> struct format_provider<dub::SourceLocation> {
  static void format(const dub::SourceLocation &loc, raw_ostream &stream,
                     StringRef style);
};

template <> struct format_provider<dub::SourceInfo> {
  static void format(const dub::SourceInfo &info, raw_ostream &stream,
                     StringRef style);
};

} // namespace llvm

#endif /* DUB_SOURCE_INFO_H_ */
