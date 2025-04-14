#ifndef LIB_H_
#define LIB_H_

#include <variant>

namespace lib {

using Bar = std::variant<int>;
template <typename Sink> void AbslStringify(Sink &sink, const Bar &b) {
  (void)b;
  sink.Append("bar");
}
struct Foo {

  // using Bar = std::variant<int, std::string>;

  template <typename Sink> friend void AbslStringify(Sink &sink, const Foo &f) {
    (void)f;
    absl::Format(&sink, "foo");
  }

  // template <typename Sink>
  // friend void AbslStringify(Sink& sink, const Foo::Bar& b) {
  //   sink.Append("bar");
  // }
};

} // namespace lib

#endif /* LIB_H_ */
