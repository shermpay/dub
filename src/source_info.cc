#include "src/source_info.h"

#include "src/form.h"
#include "src/form_format.h"

#include "llvm/Support/FormatVariadic.h"

#include <string>

namespace dub {

llvm::StringSet<> &FilenamesPool() {
  static llvm::StringSet<> pool;
  return pool;
}

std::ostream &operator<<(std::ostream &os, const SourceLocation location) {
  return os << llvm::formatv("{0}", location).str();
}

std::ostream &operator<<(std::ostream &os, const SourceInfo info) {
  return os << llvm::formatv("{0}", info).str();
}

std::string FormDebugString(const Form &form) noexcept {
  return llvm::formatv("{0} => {1}", form, *form.info).str();
}

} // namespace dub

namespace llvm {
void format_provider<dub::SourceLocation>::format(
    const dub::SourceLocation &loc, raw_ostream &stream, StringRef style) {
  (void)style;
  stream << formatv("{0}:{1},{2})-({3},{4})", loc.filename, loc.line_start,
                    loc.line_end, loc.column_start, loc.column_end);
}

void format_provider<dub::SourceInfo>::format(const dub::SourceInfo &info,
                                              raw_ostream &stream,
                                              StringRef style) {
  (void)style;
  stream << formatv("{0}:{1}", info.location, info.line);
}

} // namespace llvm
