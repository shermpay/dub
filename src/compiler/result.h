#ifndef DUB_COMPILER_RESULT_H_
#define DUB_COMPILER_RESULT_H_

#include <ostream>
#include <string_view>
#include <variant>

#include "absl/status/status.h"
#include "absl/strings/str_format.h"

namespace dub {

class Error {
 public:
  virtual ~Error() = 0;
  virtual std::string_view Message() const = 0;
  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Error& error) {
    sink.Append(error.Message());
  }
  friend std::ostream& operator<<(std::ostream& os, const Error& error) {
    os << absl::StreamFormat("%v", error);
    return os;
  }
};

class StatusError : public Error {
 public:
  StatusError(absl::Status status) : status_(status) {}
  ~StatusError() = default;

  virtual std::string_view Message() const noexcept override {
    return status_.message();
  }
 private:
  absl::Status status_;
};

template <typename T>
class Result final {
 public:
  Result(std::unique_ptr<Error> error) : value_(std::move(error)) {};
  Result(T value) : value_(std::move(value)) {};

  bool IsOk() const noexcept {
    return std::holds_alternative<T>(value_);
  }

  const T& Value() const noexcept {
    return std::get<T>(value_);
  }

  auto& value() const noexcept {
    return value_;
  }

  const Error& GetError() const noexcept {
    return *std::get<std::unique_ptr<dub::Error>>(value_);
  }

  std::string_view ErrorMessage() const noexcept {
    return std::get<std::unique_ptr<dub::Error>>(value_)->Message();
  }

 private:
  std::variant<T, std::unique_ptr<dub::Error>> value_;
};

}  // namespace dub

#endif /* DUB_COMPILER_RESULT_H_ */
