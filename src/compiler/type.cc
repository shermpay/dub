#include "src/compiler/type.h"

#include "src/symbol.h"

#include "absl/status/statusor.h"
#include "llvm/Support/FormatVariadic.h"

#include <utility>

namespace dub {

std::ostream &operator<<(std::ostream &os, const Type &type) {
  os << llvm::formatv("{0}", type).str();
  return os;
}

namespace type {

Type &Unit() {
  static Type t(Basic::kUnit);
  return t;
}

Type &Bool() {
  static Type t(Basic::kBool);
  return t;
}

Type &I8() {
  static Type t(Basic::kI8);
  return t;
}

Type &I16() {
  static Type t(Basic::kI16);
  return t;
}

Type &I32() {
  static Type t(Basic::kI32);
  return t;
}

Type &I64() {
  static Type t(Basic::kI64);
  return t;
}

std::vector<std::pair<const Symbol *, Type>> BuiltinTypes() noexcept {
  return std::vector{
      std::make_pair(&Symbol::Get("unit"), Unit()),
      std::make_pair(&Symbol::Get("bool"), Bool()),
      std::make_pair(&Symbol::Get("i8"), I8()),
      std::make_pair(&Symbol::Get("i16"), I16()),
      std::make_pair(&Symbol::Get("i32"), I32()),
      std::make_pair(&Symbol::Get("i64"), I64()),
  };
}

std::vector<std::pair<const Symbol *, Type>> BuiltinNames() noexcept {
  return std::vector{
      std::make_pair(&Symbol::Get("add-i64"),
                     Fn::MakeType(Type::Tuple({I64(), I64()}), I64())),
  };
}

class ArrayConstructor : public Constructor {
  absl::StatusOr<Type> operator()(std::vector<Type> args) const override {
    if (args.size() != 2) {
      return absl::InvalidArgumentError(
          llvm::formatv("Array constructor: invalid number of args (got {0}), "
                        "expected 2 args",
                        args.size())
              .str());
    }

    if (!args[0].Is<compiler::Constant>() ||
        !args[0].Get<compiler::Constant>().Is<std::int64_t>()) {
      return absl::InvalidArgumentError(
          llvm::formatv("Array constructor: invalid value for arg 0 (got "
                        "{0}), expected constant expression of type int64",
                        args[0])
              .str());
    }

    if (args[1].Is<compiler::Constant>()) {
      return absl::InvalidArgumentError(
          llvm::formatv("Array constructor: invalid value for arg 1 (got {0}), "
                        "expected type",
                        args[1])
              .str());
    }

    return Type(Parameterized<Type>(kArrayTag, args), Property::kNone);
  }
};

Constructor *ArrayCtor() {
  static ArrayConstructor ctor;
  return &ctor;
}

class FnConstructor : public Constructor {
  absl::StatusOr<Type> operator()(std::vector<Type> args) const override {
    if (args.size() != 2) {
      return absl::InvalidArgumentError(absl::StrFormat(
          "Fn constructor: invalid number of args (got %d), expected 2 args",
          args.size()));
    }

    return Type(Parameterized<Type>(kFnTag, args), Property::kCallable);
  }
};

Constructor *FnCtor() {
  static FnConstructor ctor;
  return &ctor;
}

class PtrConstructor : public Constructor {
  absl::StatusOr<Type> operator()(std::vector<Type> args) const override {
    if (args.size() != 1) {
      return absl::InvalidArgumentError(
          llvm::formatv("Fn constructor: invalid number of args (got {0}), "
                        "expected 1 args",
                        args.size())
              .str());
    }

    return Type(Parameterized<Type>(kPtrTag, args), Property::kNone);
  }
};

Constructor *PtrCtor() {
  static PtrConstructor ctor;
  return &ctor;
}

std::vector<std::pair<const Symbol *, Constructor *>>
BuiltinConstructors() noexcept {
  return std::vector{
      std::make_pair(&kFnTag, FnCtor()),
      std::make_pair(&kArrayTag, ArrayCtor()),
      std::make_pair(&kPtrTag, PtrCtor()),
  };
}

Type Fn::MakeType(Type::Tuple params, Type ret) {
  std::vector<Type> args;
  args.reserve(2);
  args.push_back(Type(params));
  args.push_back(ret);
  return Type(Parameterized<Type>(kFnTag, args), Property::kCallable);
}

absl::StatusOr<Fn> Fn::Get(Type::Parameterized &underlying) {
  if (*underlying.name != Symbol::Get("Fn")) {
    return absl::InvalidArgumentError("type is not Fn");
  }
  if (underlying.types.size() != 2) {
    return absl::InvalidArgumentError("type should only have two arguments");
  }
  if (!underlying.types[0].Is<Type::Tuple>()) {
    return absl::InvalidArgumentError(
        "first type argument must be a tuple type");
  }
  return Fn(underlying);
}

absl::StatusOr<Fn> Fn::Get(Type &type) {
  auto underlying = type.GetIf<Type::Parameterized>();
  if (!underlying) {
    return absl::InvalidArgumentError("type is not a parameterized type");
  }
  return Fn(*underlying);
}

Type PtrOf(Type type) {
  return Type(Parameterized<Type>(kPtrTag, {type}), Property::kNone);
}

bool IsIntegerType(Type t) {
  auto basic = t.GetIf<Basic>();
  if (basic) {
    return (*basic) == Basic::kI8 || (*basic) == Basic::kI16 ||
           (*basic) == Basic::kI32 || (*basic) == Basic::kI64;
  }
  return false;
}

} // namespace type
} // namespace dub

namespace llvm {

void format_provider<dub::Type>::format(const dub::Type &type,
                                        raw_ostream &stream, StringRef style) {
  (void)style;
  type.Match(
      [&stream](const auto &kind) { stream << llvm::formatv("{0}", kind); });
}

void format_provider<dub::type::Basic>::format(const dub::type::Basic &type,
                                               raw_ostream &stream,
                                               StringRef style) {
  (void)style;
  switch (type) {
  case dub::type::Basic::kUnit:
    stream << "unit";
    break;
  case dub::type::Basic::kBool:
    stream << "bool";
    break;
  case dub::type::Basic::kI8:
    stream << "i8";
    break;
  case dub::type::Basic::kI16:
    stream << "i16";
    break;
  case dub::type::Basic::kI32:
    stream << "i32";
    break;
  case dub::type::Basic::kI64:
    stream << "i64";
    break;
  case dub::type::Basic::kF32:
    stream << "f32";
    break;
  case dub::type::Basic::kF64:
    stream << "f64";
    break;
  }
}

void format_provider<dub::type::Parameterized<dub::Type>>::format(
    const dub::type::Parameterized<dub::Type> &type, raw_ostream &stream,
    StringRef style) {
  (void)style;
  stream << llvm::formatv("({0}", type.name);
  for (const auto &t : type.types) {
    stream << llvm::formatv(" {0}", t);
  }
  stream << ")";
}

void format_provider<dub::type::Tuple<dub::Type>>::format(
    const dub::type::Tuple<dub::Type> &type, raw_ostream &stream,
    StringRef style) {
  (void)style;
  stream << '(';
  bool first = true;
  for (const auto &t : type.types()) {
    if (first) {
      first = false;
    } else {
      stream << ' ';
    }
    stream << llvm::formatv("{0}", t);
  }
  stream << ')';
}

void format_provider<dub::type::Struct<dub::Type>::Field>::format(
    const dub::type::Struct<dub::Type>::Field &type, raw_ostream &stream,
    StringRef style) {
  stream << '(';
  format_provider<dub::Symbol>::format(*type.name, stream, style);
  stream << ' ';
  format_provider<dub::Type>::format(type.type, stream, style);
  stream << ')';
}

void format_provider<dub::type::Struct<dub::Type>>::format(
    const dub::type::Struct<dub::Type> &type, raw_ostream &stream,
    StringRef style) {
  (void)style;
  stream << "(struct [";
  bool first = true;
  for (const auto &t : type.fields()) {
    if (first) {
      first = false;
    } else {
      stream << ' ';
    }
    stream << llvm::formatv("{0}", t);
  }
  stream << "]";
}
} // namespace llvm
