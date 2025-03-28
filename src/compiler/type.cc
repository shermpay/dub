#include "src/compiler/type.h"

#include <utility>

#include "absl/status/statusor.h"

#include "src/form.h"
#include "src/symbol.h"

namespace dub::type {

Type& Unit() {
  static Type t(Basic::kUnit);
  return t;
}

Type& Bool() {
  static Type t(Basic::kBool);
  return t;
};

Type& I8() {
  static Type t(Basic::kI8);
  return t;
};

Type& I16() {
  static Type t(Basic::kI16);
  return t;
};

Type& I32() {
  static Type t(Basic::kI32);
  return t;
};

Type& I64() {
  static Type t(Basic::kI64);
  return t;
};

std::vector<std::pair<const Symbol*, Type>> BuiltinTypes() noexcept {
  return std::vector{
      std::make_pair(&Symbol::Get("unit"), Unit()),
      std::make_pair(&Symbol::Get("bool"), Bool()),
      std::make_pair(&Symbol::Get("i8"), I8()),
      std::make_pair(&Symbol::Get("i16"), I16()),
      std::make_pair(&Symbol::Get("i32"), I32()),
      std::make_pair(&Symbol::Get("i64"), I64()),
  };
}

std::vector<std::pair<const Symbol*, Type>> BuiltinNames() noexcept {
  return std::vector{
    std::make_pair(&Symbol::Get("add-i64"),
                   Fn::MakeType(Type::Tuple({I64(), I64()}), I64())),
  };
}



class ArrayConstructor : public Constructor {
  absl::StatusOr<Type> operator()(std::vector<Type> args) const override {
    if (args.size() != 2) {
      return absl::InvalidArgumentError(
          absl::StrFormat(
              "Array constructor: invalid number of args (got %d), expected 2 args",
              args.size()));
    }

    if (!args[0].Is<compiler::Constant>() ||
        !args[0].Get<compiler::Constant>().Is<std::int64_t>()) {
      return absl::InvalidArgumentError(
          absl::StrFormat(
              "Array constructor: invalid value for arg 0 (got %v), expected constant expression of type int64",
              args[0]));
    }

    if (args[1].Is<compiler::Constant>()) {
      return absl::InvalidArgumentError(
          absl::StrFormat(
              "Array constructor: invalid value for arg 1 (got %v), expected type",
              args[1]));

    }

    return Type(Parameterized<Type>(kArrayTag, args),
                Property::kNone);
  }
};

Constructor* ArrayCtor() {
  static ArrayConstructor ctor;
  return &ctor;
};


class FnConstructor : public Constructor {
  absl::StatusOr<Type> operator()(std::vector<Type> args) const override {
    if (args.size() != 2) {
      return absl::InvalidArgumentError(
          absl::StrFormat(
              "Fn constructor: invalid number of args (got %d), expected 2 args",
              args.size()));
    }

    return Type(Parameterized<Type>(kFnTag, args),
                Property::kCallable);
  }
};

Constructor* FnCtor() {
  static FnConstructor ctor;
  return &ctor;
}

class PtrConstructor : public Constructor {
  absl::StatusOr<Type> operator()(std::vector<Type> args) const override {
    if (args.size() != 1) {
      return absl::InvalidArgumentError(
          absl::StrFormat(
              "Fn constructor: invalid number of args (got %d), expected 1 args",
              args.size()));
    }

    return Type(Parameterized<Type>(kPtrTag, args),
                Property::kNone);
  }
};

Constructor* PtrCtor() {
  static PtrConstructor ctor;
  return &ctor;
}

std::vector<std::pair<const Symbol*, Constructor*>> BuiltinConstructors() noexcept {
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
};

absl::StatusOr<Fn> Fn::Get(Type::Parameterized& underlying) {
  if (*underlying.name != Symbol::Get("Fn")) {
    return absl::InvalidArgumentError("type is not Fn");
  }
  if (underlying.types.size() != 2) {
    return absl::InvalidArgumentError("type should only have two arguments");
  }
  if (!underlying.types[0].Is<Type::Tuple>()) {
    return absl::InvalidArgumentError("first type argument must be a tuple type");
  }
  return Fn(underlying);
}

absl::StatusOr<Fn> Fn::Get(Type& type) {
  auto underlying = type.GetIf<Type::Parameterized>();
  if (!underlying) {
    return absl::InvalidArgumentError("type is not a parameterized type");
  }
  return Fn(*underlying);
}


Type PtrOf(Type type) {
  return Type(Parameterized<Type>(kPtrTag, {type}),
			  Property::kNone);
}

bool IsIntegerType(Type t) {
  auto basic = t.GetIf<Basic>();
  if (basic) {
    return (*basic) == Basic::kI8 ||
        (*basic) == Basic::kI16 ||
        (*basic) == Basic::kI32 ||
        (*basic) == Basic::kI64;
  }
  return false;
}

}  // namespace dub::type
