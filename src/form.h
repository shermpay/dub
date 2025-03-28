#ifndef DUB_FORM_H_
#define DUB_FORM_H_

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "absl/strings/str_format.h"

#include "src/symbol.h"

namespace dub {

struct Nil {
  static Nil Get() {
    static Nil n;
    return n;
  };

  bool operator==(const Nil& _) const {
    return true;
  }

  bool operator!=(const Nil& _) const {
    return false;
  }
  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Nil& _) {
    sink.Append("nil");
  }
};

class MutableList;

namespace form {

/*
  Fun idea: a typesafe copy-less data driven design of forms.

  Basic idea is to cast underlying memory into structs of more and more concrete types.

  "(if foo 1 2)" =Read=> `List` =Parse=> `If`

  As such the data is the same, just how we interpret it. So we don't want to copy the data.

  How about source location? We could store a pointer to it in the form.
 */

template <typename T>
class List final {
 public:
  List(MutableList& mut_list);

  template <typename... Args>
  static List Make(Args... args) {
    std::vector<T> vals;
    (vals.push_back(args), ...);
    return List(std::make_shared<std::vector<T>>(vals), 0);
  }

  static List<T> Empty();

  auto Size() const noexcept {
    return values_->size() - head_idx_;
  };

  bool IsEmpty() const noexcept {
    return Size() == 0;
  }

  const T& operator[](std::size_t idx) const {
    return (*values_)[idx + head_idx_];
  };

  const T Head() const {
    return (*values_)[head_idx_];
  }

  const T Second() const {
    return (*values_)[head_idx_+1];
  }

  const List<T> Tail() const {
    return List(this->values_, head_idx_ + 1);
  }

  auto cbegin() const noexcept {
    return values_->cbegin() + head_idx_;
  };

  auto cend() const noexcept {
    return values_->cend();
  };

  auto begin() const noexcept {
    return cbegin();
  };

  auto end() const noexcept {
    return cend();
  };

  friend bool operator==(const List& lhs, const List& rhs) {
    return *(lhs.values_) == *(rhs.values_);
  }

  friend bool operator!=(const List& lhs, const List& rhs) {
    return !(lhs == rhs);
  }

  template <class T2>
  friend std::ostream& operator<<(std::ostream& os, const List<T2>& list);

 private:
  // shared_ptr is used for efficient Tail(),
  // without having separate types for List and ListView.
  std::shared_ptr<std::vector<T>> values_;
  std::size_t head_idx_;

  List(std::shared_ptr<std::vector<T>> values, std::size_t head_idx) :
      values_(values), head_idx_(head_idx) {};
};

template <typename Sink>
class Format {
 public:
  Format(Sink& sink) : sink_(sink) {};
  void operator()(bool x) {
    absl::Format(&sink_, "%v", x);
  };

  void operator()(std::int64_t x) {
    absl::Format(&sink_, "%lld", x);
  };

  void operator()(double x) {
    absl::Format(&sink_, "%f", x);
  };

  void operator()(std::string x) {
    sink_.Append(x);
  };

  void operator()(const auto& x) {
    AbslStringify(sink_, x);
    // absl::Format(&sink_, "%v", x);
  };
 private:
  Sink& sink_;
};

}  // namespace form

struct SourceInfo;

struct Form {
  using Vector = std::vector<Form>;
  using Value = std::variant<
      Nil,
      bool,
      std::int64_t,
      double,
      std::string,
      const Symbol*,
      form::List<Form>,
      Vector>;
  Value value;

  // Is not part of the "value" of a Form.
  SourceInfo* info;

  explicit Form(const Symbol& v) : value(&v) {}
  explicit Form(const Value& v) : value(std::move(v)) {}

  // Copy
  Form(const Form&) = default;
  Form& operator=(const Form&) = default;

  // Move
  Form(Form&&) = default;
  Form& operator=(Form&&) = default;

  template <typename T>
  T get() const {
    return std::get<T>(value);
  };

  template <typename T>
  bool is() const {
    return std::holds_alternative<T>(value);
  };

  template <typename Matcher>
  constexpr auto Match(Matcher&& m) const {
    return std::visit(m, value);
  };

  friend bool operator==(const Form& lhs, const Form& rhs) {
    return lhs.value == rhs.value;
  }

  friend bool operator!=(const Form& lhs, const Form& rhs) {
    return !(lhs == rhs);
  }
  friend std::ostream& operator<<(std::ostream& os, const Form& form);

};

// The following has to be outside the struct definition, otherwise it triggers a bug in gcc (works in clang)
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85282
template <>
const Symbol& Form::get<const Symbol&>() const;


template <typename Sink>
void AbslStringify(Sink& sink, const Form& form) {
  form.Match(form::Format(sink));
}

using List = form::List<Form>;

template <typename Sink>
void AbslStringify(Sink& sink, const List& list) {
  sink.Append("(");
  bool first = true;
  for (const auto& form : list) {
    if (first) {
      first = false;
    } else {
      sink.Append(" ");
    }
    // TODO: Use AbslStringify
    absl::Format(&sink, "%v", form);
  }
  sink.Append(")");
};



namespace form {
using ListContainer = std::vector<Form>;
}  // namespace form

class MutableList {
 public:
  form::ListContainer forms_;
  void Append(const Form& form) {
    forms_.push_back(form);
  };

 private:

  form::ListContainer& Data() {
    return forms_;
  }

  friend List::List(MutableList&);
};


Form NilForm();

using Vector = Form::Vector;

template <typename Sink>
void AbslStringify(Sink& sink, const Vector& vec) {
  sink.Append("[");
  bool first = true;
  for (const auto& form : vec) {
    if (first) {
      first = false;
    } else {
      sink.Append(" ");
    }
    absl::Format(&sink, "%v", form);
  }
  sink.Append("]");
};

}  // namespace dub


#endif /* DUB_FORM_H_ */
