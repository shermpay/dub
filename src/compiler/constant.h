#ifndef DUB_COMPILER_CONSTANT_H_
#define DUB_COMPILER_CONSTANT_H_

#include <variant>

#include "absl/strings/str_format.h"

#include "src/form.h"
#include "src/symbol.h"

namespace dub::compiler {


// Constant represents an expression that can be evaluated at compile time.
// Currently only Literals are constants.
class Constant final {
 public:
  using Literal = std::variant<
     Nil,
     bool,
     std::int64_t,
     double,
     std::string>;

  static std::string LiteralToString(const Literal& literal) {
    return std::visit([](const auto& v) {
      using T = std::decay_t<decltype(v)>;
      if constexpr (std::is_same_v<T, Nil>)
          return std::string("nil");
      else if constexpr (std::is_same_v<T, bool>)
          return absl::StrFormat("%v", v);
      else if constexpr (std::is_same_v<T, std::int64_t>)
          return absl::StrFormat("%lld", v);
      else if constexpr (std::is_same_v<T, double>)
          return absl::StrFormat("%f", v);
      else if constexpr (std::is_same_v<T, std::string>)
          return v;
    }, literal);
  }

  explicit Constant(const Literal& literal) : literal_(literal) {}
  bool operator==(const Constant& rhs) const {
    return this->literal_ == rhs.literal_;
  }

  bool operator!=(const Constant& rhs) const {
    return !(*this == rhs);
  }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Constant& cst) {
    sink.Append(LiteralToString(cst.literal_));
    // absl::Format(&sink, "%v", cst.literal_);
    //AbslStringify(sink, cst.literal_);
    // std::visit([&sink](const auto& literal) {
    //   AbslStringify(sink, Form(literal));
    // }, cst.literal_);
  }

  template <typename T>
  bool Is() const noexcept {
    return std::holds_alternative<T>(literal_);
  }

  template <typename T>
  T Get() const noexcept {
    return std::get<T>(literal_);
  }

  template <typename Matcher>
  auto Match(Matcher m) const {
    return std::visit(m, literal_);
  }

  Literal Value() const {
    return literal_;
  }

 private:
   Literal literal_;

   friend struct llvm::format_provider<Constant>;
};

}  // namespace dub::compiler

#endif /* DUB_COMPILER_CONSTANT_H_ */
