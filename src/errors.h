#ifndef DUB_ERRORS_H
#define DUB_ERRORS_H

#include <system_error>

namespace dub {
enum class ErrorCode : int {
  kParserError,
  kTypeCheckerError,
  // TODO: remove
  kStatusError,
};

inline std::error_code make_error_code(ErrorCode code) {
  return std::error_code(static_cast<int>(code), std::generic_category());
}

} // namespace dub

namespace std {
template <> struct is_error_code_enum<dub::ErrorCode> : std::true_type {};
} // namespace std

#endif // DUB_ERRORS_H
