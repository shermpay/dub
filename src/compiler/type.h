#ifndef DUB_COMPILER_TYPE_H_
#define DUB_COMPILER_TYPE_H_

#include <memory>
#include <ostream>
#include <variant>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"

#include "src/compiler/constant.h"
#include "src/form.h"
#include "src/symbol.h"

namespace dub {

namespace type {

enum class Basic {
  kUnit,
  kBool,
  kI8,
  kI16,
  kI32,
  kI64,
  kF32,
  kF64,
};

template <typename Sink>
void AbslStringify(Sink& sink, const Basic& type) {
  switch (type) {
    case Basic::kUnit:
      sink.Append("unit");
      break;
    case Basic::kBool:
      sink.Append("bool");
      break;
    case Basic::kI8:
      sink.Append("i8");
      break;
    case Basic::kI16:
      sink.Append("i16");
      break;
    case Basic::kI32:
      sink.Append("i32");
      break;
    case Basic::kI64:
      sink.Append("i64");
      break;
    case Basic::kF32:
      sink.Append("f32");
      break;
    case Basic::kF64:
      sink.Append("f64");
      break;
  }
}


// TODO: Is this too generic?
//  What about the following:
//
// struct Parameterized {
//   std::variant<Function, Array, Pointer>
// }

template <class T>
struct Parameterized {
  const Symbol* name;
  std::vector<T> types;
  explicit Parameterized(const Symbol& name, std::vector<T> ts) :
      name(&name), types(ts) {};

  explicit Parameterized(const Symbol& name, T t) :
      name(&name), types(std::vector<T>()) {
    types.push_back(t);
  };

  template <typename... Args>
  static Parameterized Make(Symbol& name, Args... args) {
    std::vector<T> ts;
    (ts.push_back(args), ...);
    return Parameterized(name, std::vector<T>(ts));
  }

  bool operator==(const Parameterized& rhs) const {
    return name == rhs.name && types == rhs.types;
  }

  bool operator!=(const Parameterized& rhs) const {
    return !(*this == rhs);
  }
};

template <class T>
class Tuple final {
 public:
  Tuple(std::vector<T> types) : types_(types) {}

  bool operator==(const Tuple& rhs) const {
    return types_ == rhs.types_;
  }

  bool operator!=(const Tuple& rhs) const {
    return !(*this == rhs);
  }

  const absl::Span<const T> types() const noexcept {
    return types_;
  }

 private:
  std::vector<T> types_;
};

template <class T>
class Struct final {
 public:
  struct Field {
	const Symbol* name;
	T type;

	bool operator==(const Field& rhs) const {
	  return name == rhs.name && type == rhs.type;
	}

	bool operator!=(const Field& rhs) const {
	  return !(*this == rhs);
	}
  };
  bool operator==(const Struct& rhs) const {
    return fields_ == rhs.fields_;
  }

  bool operator!=(const Struct& rhs) const {
    return !(*this == rhs);
  }

  void AddField(const Symbol& name, T type) {
	fields_.push_back({&name, type});
  }

  const absl::Span<const Field> fields() const {
	return fields_;
  }

 private:
  std::vector<Field> fields_;
};

enum class Property {
  kNone,
  kCallable,
};

}  // namespace type

class Type final {
 public:
  using Parameterized = type::Parameterized<Type>;
  using Tuple = type::Tuple<Type>;
  using Struct = type::Struct<Type>;

  explicit Type(type::Basic t) : kind_(t), prop_(type::Property::kNone)  {};
  explicit Type(type::Parameterized<Type> ts, type::Property p) : kind_(ts), prop_(p) {};
  explicit Type(compiler::Constant c) : kind_(c), prop_(type::Property::kNone) {};
  explicit Type(type::Tuple<Type> ts) : kind_(ts), prop_(type::Property::kNone) {};
  explicit Type(Struct st) : kind_(st), prop_(type::Property::kNone) {};

  bool operator==(const Type& rhs) const {
    return kind_ == rhs.kind_ && prop_ == rhs.prop_;
  }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Type type) {
    std::visit([&sink](const auto& kind) {
      AbslStringify(sink, kind);
    }, type.kind_);
  }

  friend std::ostream& operator<<(std::ostream& os, const Type& type) {
    os << absl::StreamFormat("%v", type);
    return os;
  }

  bool IsCallable() const {
    return prop_ == type::Property::kCallable;
  }

  template <typename T>
  bool Is() const noexcept {
    return std::holds_alternative<T>(kind_);
  }

  template <typename T>
  T Get() const noexcept {
    return std::get<T>(kind_);
  }

  template <typename T>
  T* GetIf() noexcept {
    return std::get_if<T>(&kind_);
  }

  template <typename Matcher>
  auto Match(Matcher m) const {
    return std::visit(m, kind_);
  }

 private:
  std::variant<type::Basic, Parameterized, compiler::Constant, Tuple, Struct> kind_;
  type::Property prop_;
};


namespace type {

template <typename Sink>
void AbslStringify(Sink& sink, const Parameterized<Type> type) {
  absl::Format(&sink, "(%v", type.name);
  for (const auto& t : type.types) {
    absl::Format(&sink, " %v", t);
  }
  sink.Append(")");
};

template <typename Sink>
void AbslStringify(Sink& sink, const Tuple<Type> type) {
  sink.Append("(");
  bool first = true;
  for (const auto& t : type.types()) {
    if (first) {
      first = false;
    } else {
      sink.Append(" ");
    }
    absl::Format(&sink, "%v", t);
  }
  sink.Append(")");
};

template <typename Sink>
void AbslStringify(Sink& sink, const Struct<Type>::Field type) {
  sink.Append("(");
  AbslStringify(sink, type.name);
  sink.Append(" ");
  AbslStringify(sink, type.type);
  sink.Append(")");
}

template <typename Sink>
void AbslStringify(Sink& sink, const Struct<Type> type) {
  sink.Append("(struct [");
  bool first = true;
  for (const auto& t : type.fields()) {
    if (first) {
      first = false;
    } else {
      sink.Append(" ");
    }
    absl::Format(&sink, "%v", t);
  }
  sink.Append("])");
};

Type& Unit();
Type& Bool();
Type& I8();
Type& I16();
Type& I32();
Type& I64();

std::vector<std::pair<const Symbol*, Type>> BuiltinTypes() noexcept;
std::vector<std::pair<const Symbol*, Type>> BuiltinNames() noexcept;

static const Symbol& kFnTag = Symbol::Get("Fn");
static const Symbol& kArrayTag = Symbol::Get("Array");
static const Symbol& kPtrTag = Symbol::Get("Ptr");

// Constructor is not a dub type, but it is a function of types -> type.
class Constructor {
 public:
  virtual absl::StatusOr<Type> operator()(std::vector<Type> args) const = 0;
};

Constructor* ArrayCtor();
Constructor* FnCtor();
Constructor* PtrCtor();
std::vector<std::pair<const Symbol*, Constructor*>> BuiltinConstructors() noexcept;


class Fn final {
 public:
  static absl::StatusOr<Fn> Get(Type::Parameterized& underlying);
  static absl::StatusOr<Fn> Get(Type& type);
  static Type MakeType(Type::Tuple params, Type ret);

  Type::Tuple ParamTypes() const {
    return underlying_.types[0].Get<Type::Tuple>();
  };
  Type ReturnType() const {
    return underlying_.types[1];
  }

 private:
  Fn(Type::Parameterized& underlying) : underlying_(underlying) {}
  Type::Parameterized& underlying_;
};

Type PtrOf(Type type);

bool IsIntegerType(Type t);
bool IsFnType(Type t);
bool IsArrayType(Type t);

}  // namespace type

}  // namespace dub

#endif /* DUB_COMPILER_TYPE_H_ */
