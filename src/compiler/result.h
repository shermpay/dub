#ifndef DUB_COMPILER_RESULT_H_
#define DUB_COMPILER_RESULT_H_

#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>
#include <vector>

namespace dub {

/// Result holds the result information of a compilation "phase".
/// A successful Result contains no errors.
/// The purpose is to try and accumulate as many errors as possible.
/// The plan is to only terminate the compilation when a phase completes with
/// errors, or the compiler encounters a "unproceedable" error.
template <typename T> class Result final {
public:
  Result(T value) : value_(std::optional(std::move(value))) {}
  Result(llvm::Error err) { AddError(std::move(err)); }
  Result(std::vector<llvm::Error> &errs) : errors_(std::move(errs)) {}

  static Result<T> Unproceedable(llvm::Error err) {
    Result<T> r(std::move(err));
    r.terminal_ = true;
    return r;
  }

  static Result<T> Unproceedable(std::vector<llvm::Error> &errs) {
    Result<T> r(errs);
    r.terminal_ = true;
    return r;
  }

  template <typename ErrT> static Result<T> Unproceedable(ErrT err) {
    static_assert(std::is_base_of_v<llvm::ErrorInfo<ErrT>, ErrT>,
                  "ErrT must derive from llvm::ErrorInfo");
    Result<T> r(llvm::make_error<ErrT>(err));
    r.terminal_ = true;
    return r;
  }

  auto &value() const noexcept {
    assert(value_.has_value());
    return value_.value();
  }

  auto &errors() const noexcept { return errors_; }

  bool IsOk() const noexcept { return errors_.empty(); }

  T &Value() & noexcept {
    assert(value_.has_value());
    return value_.value();
  }

  const T &Value() const & noexcept {
    assert(value_.has_value());
    return value_.value();
  }

  const T &&Value() const && noexcept {
    assert(value_.has_value());
    return value_.value();
  }

  auto &errors() noexcept { return errors_; }

  void AddError(llvm::Error &&err) { errors_.push_back(std::move(err)); }
  void AddAllErrors(std::vector<llvm::Error> &others) {
    errors_.reserve(errors_.size() + others.size());
    errors_.insert(errors_.cend(), std::make_move_iterator(others.begin()),
                   std::make_move_iterator(others.end()));
  }

  bool HasError() const noexcept { return !errors_.empty(); }

  void LogErrors(llvm::raw_ostream &stream) {
    for (auto &err : errors_) {
      stream << err;
    }
  }

  std::string FormatErrors() {
    std::string s;
    llvm::raw_string_ostream st(s);
    LogErrors(st);
    return s;
  }

private:
  std::optional<T> value_;
  std::vector<llvm::Error> errors_;
  bool terminal_;
};

} // namespace dub

#endif /* DUB_COMPILER_RESULT_H_ */
