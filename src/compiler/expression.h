#ifndef DUB_COMPILER_EXPRESSION_H_
#define DUB_COMPILER_EXPRESSION_H_

#include <iostream>
#include <optional>
#include <variant>

#include "absl/strings/str_format.h"
#include "absl/types/span.h"

#include "src/compiler/constant.h"
#include "src/form.h"
#include "src/symbol.h"

namespace dub {

class Expression;

template <typename ExprT = Expression> class ExprBase {
public:
  template <typename Sink>
  friend void AbslStringify(Sink &sink, const ExprBase<ExprT> &expr) {
    absl::Format(&sink, "(%v", expr.Id());
    for (const auto &expr : expr.SubExprs()) {
      absl::Format(&sink, " %v", expr);
    }
    sink.Append(")");
  }
  friend std::ostream &operator<<(std::ostream &os,
                                  const ExprBase<ExprT> &expr) {
    os << absl::StreamFormat("%v", expr);
    return os;
  }

private:
  virtual const Symbol &Id() const = 0;
  virtual const absl::Span<const ExprT> SubExprs() const = 0;
};

template <typename ExprT = Expression>
class Call final : public ExprBase<ExprT> {
public:
  explicit Call(ExprT target, std::vector<ExprT> args) {
    exprs_.reserve(args.size() + 1);
    exprs_.push_back(std::move(target));
    exprs_.insert(exprs_.cend(), std::make_move_iterator(args.begin()),
                  std::make_move_iterator(args.end()));
  }
  explicit Call(ExprT target, std::convertible_to<ExprT> auto... args) {
    exprs_.push_back(std::move(target));
    (this->exprs_.push_back(std::move(args)), ...);
  }

  // Movable
  // Call(Call&& other) = default;
  // Call& operator=(Call&& other) = default;

  bool operator==(const Call<ExprT> &rhs) const { return exprs_ == rhs.exprs_; }

  bool operator!=(const Call<ExprT> &rhs) const { return !(*this == rhs); };

  friend std::ostream &operator<<(std::ostream &os, const Call<ExprT> &expr) {
    os << absl::StreamFormat("%v", expr);
    return os;
  }

  const ExprT &target() const { return exprs_[0]; }

  const absl::Span<const ExprT> args() const {
    return absl::MakeSpan(exprs_).subspan(1);
  }

private:
  const Symbol &Id() const override { return Symbol::Get("call"); };

  const absl::Span<const ExprT> SubExprs() const override { return exprs_; }

  std::vector<ExprT> exprs_;
};

template <typename ExprT = Expression> class If final : public ExprBase<ExprT> {
public:
  If() : exprs_(std::make_unique<std::array<ExprT, 3>>()) {};

  If(ExprT cond, ExprT then, ExprT els)
      : exprs_(std::make_unique<std::array<ExprT, 3>>()) {
    (*exprs_)[0] = std::move(cond);
    (*exprs_)[1] = std::move(then);
    (*exprs_)[2] = std::move(els);
  };

  // Moveable
  // If(If&&) = default;
  // If& operator=(If&&) = default;

  bool operator==(const If<ExprT> &rhs) const {
    return cond() == rhs.cond() && then() == rhs.then() && els() == rhs.els();
  }

  bool operator!=(const If<ExprT> &rhs) const { return !(*this == rhs); };

  const ExprT &cond() const { return (*exprs_)[0]; }

  const ExprT &then() const { return (*exprs_)[1]; }

  const ExprT &els() const { return (*exprs_)[2]; }

private:
  const Symbol &Id() const override { return Symbol::Get("if"); };

  const absl::Span<const ExprT> SubExprs() const override { return *exprs_; }

  std::unique_ptr<std::array<ExprT, 3>> exprs_;
};

template <typename ExprT> class Return final : public ExprBase<ExprT> {
public:
  Return() : value_(nullptr) {}
  explicit Return(std::unique_ptr<ExprT> value) : value_(std::move(value)) {}
  bool operator==(const Return<ExprT> &rhs) const {
    if (value_ && rhs.value_) {
      return *value_ == *rhs.value_;
    }
    return !value_ && !rhs.value_;
  }
  bool operator!=(const Return<ExprT> &rhs) const { return !(*this == rhs); };

  bool HasValue() const { return bool(value_); }

  const Expression &Value() const { return *value_; }

private:
  const Symbol &Id() const override { return Symbol::Get("return"); };

  const absl::Span<const ExprT> SubExprs() const override {
    if (value_) {
      return absl::MakeSpan(value_.get(), 1);
    } else {
      return absl::Span<const ExprT>();
    }
  }

  std::unique_ptr<ExprT> value_;
};

/*
  (type foo (Fn [i64 i64] i64))
  (fn foo [x y]
  (+ (x y))
*/
template <typename ExprT = Expression> class Fn final {
public:
  Fn() : params_(std::vector<const Symbol *>()), body_(std::vector<ExprT>()) {};
  Fn(const Symbol &name, std::vector<const Symbol *> &params,
     std::vector<ExprT> body)
      : name_(&name), params_(std::move(params)), body_(std::move(body)) {};

  // Moveable
  // Fn(Fn&&) = default;
  // Fn& operator=(Fn&&) = default;

  bool operator==(const Fn<ExprT> &rhs) const {
    return name_ == rhs.name_ && params_ == rhs.params_ && body_ == rhs.body_;
  }

  bool operator!=(const Fn<ExprT> &rhs) const { return !(*this == rhs); };

  friend std::ostream &operator<<(std::ostream &os, const Fn<ExprT> &fn) {
    os << absl::StreamFormat("%v", fn);
    return os;
  };

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const Fn<ExprT> &fn);

  void SetName(const Symbol &name) { name_ = &name; };

  const Symbol *name() const { return name_; }

  void SetParams(std::vector<const Symbol *> params) {
    params_ = std::move(params);
  };

  absl::Span<const Symbol *const> params() const { return params_; }

  void SetBody(std::vector<ExprT> &body) { body_ = std::move(body); }

  absl::Span<const ExprT> body() const { return body_; }

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const Fn<ExprT> &fn) {
    sink.Append("(fn ");
    if (fn.name_ != nullptr) {
      absl::Format(&sink, "%v ", *fn.name_);
    }

    sink.Append("[");
    bool first = true;
    for (const auto &sym : fn.params_) {
      if (first) {
        first = false;
      } else {
        sink.Append(" ");
      }
      absl::Format(&sink, "%v", *sym);
    }
    sink.Append("]");
    for (const auto &expr : fn.body_) {
      absl::Format(&sink, " %v", expr);
    }
    sink.Append(")");
  };

private:
  const Symbol *name_;
  std::vector<const Symbol *> params_;
  std::vector<ExprT> body_;
};

template <typename ExprT = Expression> class Array final {
public:
  Array(std::vector<ExprT> &&exprs) : exprs_(std::move(exprs)) {};

  bool operator==(const Array<ExprT> &rhs) const {
    return this->exprs_ == rhs.exprs_;
  }

  bool operator!=(const Array<ExprT> &rhs) const { return !(*this == rhs); };

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const Array<ExprT> &expr) {
    sink.Append("[");
    bool first = true;
    for (const auto &expr : expr.exprs_) {
      if (first)
        first = false;
      else
        sink.Append(" ");

      AbslStringify(sink, expr);
    }
    sink.Append("]");
  }

  friend std::ostream &operator<<(std::ostream &os, const Array<ExprT> &expr) {
    os << absl::StreamFormat("%v", expr);
    return os;
  }

private:
  std::vector<ExprT> exprs_;
};

namespace compiler {

class ExprInfo;

} // namespace compiler

namespace special {

static const Symbol &kFn = Symbol::Get("fn");
static const Symbol &kIf = Symbol::Get("if");
static const Symbol &kReturn = Symbol::Get("return");

static const Symbol &kModule = Symbol::Get("module");
static const Symbol &kType = Symbol::Get("type");
static const Symbol &kDeclare = Symbol::Get("declare");
static const Symbol &kVar = Symbol::Get("var");
static const Symbol &kSet = Symbol::Get("set");

absl::Span<const Symbol *const> Symbols();

} // namespace special

// TypeExpr is a type expression.
//  - Symbol: unit, bool, i64 etc.
//  - List: for type construction: `(Array 3 i64)`
//  - Vector: for type tuples [i64 i32]. eg. `(Fn [i64 bool] i32)`
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
        : name_(&name), types_(types) {};

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

    const Symbol &name() const { return *name_; };

    absl::Span<const TypeExpr> types() const { return types_; }

  private:
    const Symbol *name_;
    std::vector<TypeExpr> types_;
  };
  struct Tuple {
    std::vector<TypeExpr> types;

    Tuple(std::vector<TypeExpr> types) : types(types) {};

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

    const absl::Span<const NameType> fields() const;

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

class NameDecl final {
public:
  NameDecl() : name_(&Symbol::Get("")), type_(Symbol::Get("")) {};
  NameDecl(const Symbol &name, TypeExpr &type)
      : name_(&name), type_(std::move(type)) {};

  bool operator==(const NameDecl &rhs) const {
    return name_ == rhs.name_ && type_ == rhs.type_;
  }

  bool operator!=(const NameDecl &rhs) const { return !(*this == rhs); }

  friend std::ostream &operator<<(std::ostream &os, const NameDecl &def) {
    os << absl::StreamFormat("%v", def);
    return os;
  }

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const NameDecl &def) {
    absl::Format(&sink, "(declare %v %v)", def.name_, def.type_);
  }

  const Symbol &name() const { return *name_; }

  const TypeExpr &type() const { return type_; }

private:
  const Symbol *name_;
  TypeExpr type_;
};

TypeExpr TypeUnit();

class TypeDef final {
public:
  TypeDef() : name_(&Symbol::Get("")), type_(Symbol::Get("")) {};
  TypeDef(const Symbol &name, TypeExpr &type)
      : name_(&name), type_(std::move(type)) {};

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

template <typename ExprT> class VarDef final {
public:
  VarDef() : type_(TypeUnit()) {}
  VarDef(const Symbol &name, TypeExpr type) : name_(&name), type_(type) {}
  VarDef(const Symbol &name, TypeExpr type, std::unique_ptr<ExprT> value)
      : name_(&name), type_(type), init_(std::move(value)) {}

  bool operator==(const VarDef &rhs) const {
    return name_ == rhs.name_ && type_ == rhs.type_ && *init_ == *rhs.init_;
  }

  bool operator!=(const VarDef &rhs) const { return !(*this == rhs); }
  friend std::ostream &operator<<(std::ostream &os, const VarDef &var) {
    os << absl::StreamFormat("%v", var);
    return os;
  }

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const VarDef &var) {
    absl::Format(&sink, "(var %v %v", var.name_, var.type_);
    if (var.init_) {
      absl::Format(&sink, " %v", *var.init_);
    }
    sink.Append(")");
  }

  const Symbol &name() const { return *name_; }

  const TypeExpr &type() const { return type_; }

private:
  const Symbol *name_;
  TypeExpr type_;
  std::unique_ptr<ExprT> init_;
};

template <typename ExprT> class MemberAccess final {
public:
  MemberAccess() {}
  MemberAccess(std::unique_ptr<ExprT> struct_expr, const Symbol &member)
      : struct_expr_(std::move(struct_expr)), member_(&member) {}

  bool operator==(const MemberAccess &rhs) const {
    return member_ == rhs.member_ && *struct_expr_ == *rhs.struct_expr_;
  }

  bool operator!=(const MemberAccess &rhs) const { return !(*this == rhs); }
  friend std::ostream &operator<<(std::ostream &os,
                                  const MemberAccess &access) {
    os << absl::StreamFormat("%v", access);
    return os;
  }

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const MemberAccess &expr) {
    absl::Format(&sink, "(. %v %v)", *expr.struct_expr_, expr.member_);
  }

  const ExprT &struct_expr() const { return *struct_expr_; }

  const Symbol &member() const { return *member_; }

private:
  std::unique_ptr<ExprT> struct_expr_;
  const Symbol *member_;
};

// template <typename ExprT>
// class Assignable final {
//  public:
//   Assignable(const Symbol& name) : kind_(&name) {}

//   friend std::ostream& operator<<(std::ostream& os, const Assignable& expr) {
//     os << absl::StreamFormat("%v", expr);
//     return os;
//   }

//   template <typename Sink>
//   friend void AbslStringify(Sink& sink, const Assignable& expr) {
//     std::visit([&sink](const auto& kind) {
//       AbslStringify(sink, kind);
//     }, expr.kind_);
//   }

//  private:
// using Assignable = std::variant<const Symbol*, MemberAccess<ExprT>> kind_;
// };

using Assignable = std::variant<const Symbol *, MemberAccess<Expression>>;
template <typename Sink>
void AbslStringify(Sink &sink, const Assignable &expr) {
  std::visit([&sink](const auto &kind) { AbslStringify(sink, kind); }, expr);
};

template <typename... Ts> struct AssignableVisitor : Ts... {
  using Ts::operator()...;
};

template <typename ExprT> class Set final {
public:
  Set() {}
  Set(Assignable place, std::unique_ptr<ExprT> value)
      : place_(std::move(place)), value_(std::move(value)) {}

  bool operator==(const Set &rhs) const {
    return place_ == rhs.place_ && *value_ == *rhs.value_;
  }

  bool operator!=(const Set &rhs) const { return !(*this == rhs); }
  friend std::ostream &operator<<(std::ostream &os, const Set &expr) {
    os << absl::StreamFormat("%v", expr);
    return os;
  }

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const Set &expr) {
    absl::Format(&sink, "(set %v %v)", expr.place_, *expr.value_);
  }

  const Assignable &place() const { return place_; }

  const ExprT &value() const { return *value_; }

private:
  Assignable place_;
  std::unique_ptr<ExprT> value_;
};

struct ModuleDecl final {
  const Symbol *name;

  // Constructs an "unnamed" module
  explicit ModuleDecl() {};

  explicit ModuleDecl(const Symbol &name) : name(&name) {};

  // Move only
  // ModuleDecl(ModuleDecl&&) = default;
  // ModuleDecl& operator=(ModuleDecl&&) = default;

  bool operator==(const ModuleDecl &rhs) const { return name == rhs.name; }

  bool operator!=(const ModuleDecl &rhs) const { return !(*this == rhs); };
  friend std::ostream &operator<<(std::ostream &os, const ModuleDecl &header) {
    os << "(module " << header.name << ")";
    return os;
  }

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const ModuleDecl &expr) {
    sink.Append("(module ");
    absl::Format(&sink, "%v", *expr.name);
    sink.Append(")");
  };

  void SetName(const Symbol &name) noexcept { this->name = &name; };
};

struct ExprId final {
  const Symbol *module_name;
  std::uint64_t id;

  static ExprId Default() { return ExprId{nullptr, 0}; }

  std::size_t ToKey() const {
    // TODO: Add module name
    return id;
  }

  friend bool operator==(const ExprId &lhs, const ExprId &rhs) {
    // TODO: Add module name
    return lhs.id == rhs.id;
  }
  template <typename H> friend H AbslHashValue(H h, const ExprId &m) {
    // TODO: Add module name
    return H::combine(std::move(h), m.id);
  }
};

// TODO: Rename to Node
// Expression represents an AST node.
class Expression final {
public:
  using Kind =
      std::variant<compiler::Constant, const Symbol *, Call<Expression>,
                   If<Expression>, Fn<Expression>, Return<Expression>,
                   Array<Expression>, VarDef<Expression>, Set<Expression>,
                   MemberAccess<Expression>, ModuleDecl, TypeDef, NameDecl>;

  Expression() : kind_(compiler::Constant(Nil::Get())) {}

  // Non-copyable
  Expression(const Expression &) = delete;
  Expression &operator=(const Expression &&) = delete;

  // Movable
  Expression(Expression &&) = default;
  Expression &operator=(Expression &&) = default;

  // Constructs an "unattached" Expression.
  explicit Expression(Kind kind)
      : kind_(std::move(kind)), id_(ExprId::Default()) {}

  Expression(Kind &kind, const ExprId id) : kind_(std::move(kind)), id_(id) {}
  Expression(Kind &&kind, const ExprId id) : kind_(std::move(kind)), id_(id) {}

  static Expression Literal(compiler::Constant x, const ExprId id) {
    return Expression(x, id);
  }

  static Expression Name(const Symbol *x, const ExprId id) {
    return Expression(x, id);
  }

  bool operator==(const Expression &rhs) const {
    return this->kind_ == rhs.kind_;
  };

  bool operator!=(const Expression &rhs) const { return !(*this == rhs); };

  template <typename Sink>
  friend void AbslStringify(Sink &sink, const Expression &expr) {
    std::visit(
        [&sink](const auto &kind) -> auto { return AbslStringify(sink, kind); },
        expr.kind_);
  };

  friend std::ostream &operator<<(std::ostream &os,
                                  const Expression &expression) {
    os << absl::StreamFormat("%v", expression);
    return os;
  }

  Kind &kind() { return kind_; };

  template <typename T> bool Is() const noexcept {
    return std::holds_alternative<T>(kind_);
  }

  template <typename T> T Get() const { return std::get<T>(kind_); }

  template <typename T> T *GetIf() { return std::get_if<T>(kind_); }

  template <typename Matcher> constexpr auto Match(Matcher &&f) const {
    return std::visit(f, kind_);
  };

  template <typename Matcher> constexpr auto MutableMatch(Matcher &&f) {
    return std::visit(f, kind_);
  };

  const ExprId &Id() const noexcept { return id_; }

  std::optional<Assignable> AsAssignable() {
    return std::visit(
        [&]<typename T>(T &&v) -> std::optional<Assignable> {
		  // TODO: get types from Assignable
          if constexpr (std::disjunction_v<
						std::is_same<std::decay_t<T>, const Symbol *>,
						std::is_same<std::decay_t<T>, MemberAccess<Expression>>>)
            return std::move(v);
          else
            return std::nullopt;
        },
        kind_);
  }

private:
  Kind kind_;
  ExprId id_;
};

} // namespace dub

#endif /* DUB_EXPRESSION_H_ */
