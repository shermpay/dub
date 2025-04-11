#include "src/lib.h"

#include <iostream>
#include <optional>
#include <variant>

#include "absl/strings/str_format.h"


class Base {
 public:
  virtual std::string Name() const noexcept {
    return absl::StrFormat("name:%d", Id());
  }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Base& x) {
    absl::Format(&sink, "string:%d", x.Id());
  }
 private:
  virtual int Id() const noexcept = 0;
};

class Derived : public Base {
  int Id() const noexcept override {
    return 42;
  }
};

int main() {
  std::cout << absl::StreamFormat("%v", Derived()) << std::endl;
  return 0;
}
