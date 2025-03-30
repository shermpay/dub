#ifndef DUB_SYMBOL_H_
#define DUB_SYMBOL_H_

#include <cassert>
#include <memory>
#include <string_view>
#include <string>

#include "absl/container/node_hash_map.h"
#include "absl/strings/str_format.h"

namespace dub {

class Symbol final {
 private:
  struct CtorKey{};
 public:
  Symbol(std::string value, CtorKey) : value_(value) {}

  // Non-copyable
  Symbol(const Symbol&) = delete;
  Symbol& operator=(const Symbol&) = delete;

  // Non-movable
  // (Cannot be stored in flat_hash_map or std::unordered_map)
  Symbol(Symbol&&) = delete;
  Symbol& operator=(Symbol&&) = delete;

  ~Symbol() = default;

  static absl::node_hash_map<std::string, Symbol>& Interns() {
    static absl::node_hash_map<std::string, Symbol> interns;
    return interns;
  }

  /// Returns a reference to a interned Symbol, constructs and interns the Symbol if it isn't interned.
  static const Symbol& Get(std::string name);

  std::string_view value() const {
    return value_;
  }

  std::size_t hash() const noexcept {
    return reinterpret_cast<std::size_t>(this);
  }

  friend bool operator==(const Symbol& lhs, const Symbol& rhs) {
    return lhs.hash() == rhs.hash();
  }

  friend bool operator!=(const Symbol& lhs, const Symbol& rhs) {
    return !(lhs == rhs);
  }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Symbol& p) {
    absl::Format(&sink, "%s", p.value_);
  }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Symbol* p) {
    absl::Format(&sink, "%s", p->value_);
  }

  // template <typename H>
  // friend H AbslHashValue(H h, const Symbol& s) {
  //   return H::combine(std::move(h), s.hash());
  // }

 private:
  std::string value_;
};

std::ostream& operator<<(std::ostream& os, const Symbol& symbol);

}  // namespace dub

// template <>
// struct std::hash<dub::Symbol> {
//   std::size_t operator()(const dub::Symbol& sym) {
//     return sym.hash();
//   };
// };

#endif /* DUB_SYMBOL_H_ */
