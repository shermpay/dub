#ifndef DUB_SOURCE_INFO_H_
#define DUB_SOURCE_INFO_H_

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

  friend std::ostream &operator<<(std::ostream &os,
                                  const SourceLocation location);
};

struct SourceInfo final {
  SourceLocation location;
  // TODO: Update the following
  std::string_view line;
  // List* list;

  friend std::ostream &operator<<(std::ostream &os, const SourceInfo info);
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
