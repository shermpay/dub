#ifndef DUB_COMPILER_TYPE_EXPRS_H
#define DUB_COMPILER_TYPE_EXPRS_H

#include "src/compiler/constant.h"
#include "src/symbol.h"

#include "llvm/ADT/ArrayRef.h"

namespace dub {

/// TypeExpr is a type expression.
///  - Symbol: unit, bool, i64 etc.
///  - List: for type construction: `(Array 3 i64)`
///  - Vector: for type tuples [i64 i32]. eg. `(Fn [i64 bool] i32)`
class TypeExpr final {
public:
  // struct NameType final {
  // 	const Symbol* name;
  // 	TypeExpr type;
  // 	bool operator==(const NameType& rhs) const {
  // 	  return name == rhs.name && type == rhs.type;
  // 	}
  // 	bool operator!=(const NameType& rhs) const {
  // 	  return !(*this == rhs);
  // 	}
  // 	template <typename Sink>
  // 	friend void AbslStringify(Sink& sink, const NameType& pair) {
  // 	  absl::Format(&sink, "(%v %v)", *pair.name, pair.type);
  // 	}
  // };
  using NameType = std::pair<const Symbol *, TypeExpr>;

  class Construct final {
  public:
    Construct(const Symbol &name, std::vector<TypeExpr> types)
        : name_(&name), types_(types) {}

    bool operator==(const Construct &rhs) const {
      return this->name_ == rhs.name_ && this->types_ == rhs.types_;
    }

    bool operator!=(const Construct &rhs) const { return !(*this == rhs); }

    template <typename Sink>
    friend void AbslStringify(Sink &sink, const Construct &type) {
      absl::Format(&sink, "(%v", type.name_);
      for (const auto &type : type.types_) {
        absl::Format(&sink, " %v", type);
      }
      sink.Append(")");
    }

    const Symbol &name() const { return *name_; }

    llvm::ArrayRef<TypeExpr> types() const { return types_; }

  private:
    const Symbol *name_;
    std::vector<TypeExpr> types_;
  };
  struct Tuple {
    std::vector<TypeExpr> types;

    Tuple(std::vector<TypeExpr> types) : types(types) {}

    bool operator==(const Tuple &rhs) const { return this->types == rhs.types; }

    bool operator!=(const Tuple &rhs) const { return !(*this == rhs); }
    template <typename Sink>
    friend void AbslStringify(Sink &sink, const Tuple &type) {
      sink.Append("[");
      bool first = true;
      for (const auto &type : type.types) {
        if (first) {
          first = false;
          absl::Format(&sink, "%v", type);
        } else {
          absl::Format(&sink, " %v", type);
        }
      }
      sink.Append("]");
    }
  };
  class Struct final {
  public:
    Struct();
    Struct(std::vector<TypeExpr::NameType> fields);

    bool operator==(const Struct &rhs) const;
    bool operator!=(const Struct &rhs) const { return !(*this == rhs); }

    const llvm::ArrayRef<NameType> fields() const;

  private:
    std::vector<TypeExpr::NameType> fields_;
  };

  TypeExpr(compiler::Constant value) : kind_(value) {}
  TypeExpr(const Symbol &name) : kind_(&name) {}
  TypeExpr(const Symbol &name, std::vector<TypeExpr> types)
      : kind_(Construct(name, types)) {}
  TypeExpr(std::vector<TypeExpr> types) : kind_(Tuple(types)) {}
  TypeExpr(Struct struct_def);

  bool operator==(const TypeExpr &rhs) const;
  bool operator!=(const TypeExpr &rhs) const { return !(*this == rhs); }

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const TypeExpr &type) {
    std::visit([&sink](const auto &kind) { AbslStringify(sink, kind); },
               type.kind_);
  }

  template <typename Matcher> constexpr auto Match(Matcher &&m) const {
    return std::visit(m, kind_);
  }

private:
  std::variant<compiler::Constant, const Symbol *, Construct, Tuple, Struct>
      kind_;
};

template <typename Sink>
void AbslStringify(Sink &sink, const TypeExpr::NameType &pair) {
  absl::Format(&sink, "(%v %v)", *pair.first, pair.second);
}

template <typename Sink>
void AbslStringify(Sink &sink, const TypeExpr::Struct &st) {
  sink.Append("(struct [");
  bool first = true;
  for (const auto &field_def : st.fields()) {
    if (first) {
      first = false;
    } else {
      sink.Append(" ");
    }
    AbslStringify(sink, field_def);
  }
  sink.Append("])");
}

TypeExpr TypeUnit();

class TypeDef final {
public:
  TypeDef() : name_(&Symbol::Get("")), type_(Symbol::Get("")) {}
  TypeDef(const Symbol &name, TypeExpr &type)
      : name_(&name), type_(std::move(type)) {}

  bool operator==(const TypeDef &rhs) const {
    return name_ == rhs.name_ && type_ == rhs.type_;
  }

  bool operator!=(const TypeDef &rhs) const { return !(*this == rhs); }

  friend std::ostream &operator<<(std::ostream &os, const TypeDef &type) {
    os << absl::StreamFormat("%v", type);
    return os;
  }

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const TypeDef &type) {
    absl::Format(&sink, "(type %v %v)", type.name_, type.type_);
  }

  const Symbol &name() const { return *name_; }

  const TypeExpr &type() const { return type_; }

private:
  const Symbol *name_;
  TypeExpr type_;
};

} // namespace dub

#endif
