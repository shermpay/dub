#ifndef DUB_COMPILER_EXPRESSION_H_
#define DUB_COMPILER_EXPRESSION_H_

#include "src/compiler/constant.h"
#include "src/compiler/type_exprs.h"
#include "src/form.h"
#include "src/symbol.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/FormatVariadicDetails.h"

#include <optional>
#include <ostream>
#include <variant>

namespace dub {

class Expression;

namespace exprs {

template <typename ExprT = Expression> class ExprBase {
public:
  friend std::ostream &operator<<(std::ostream &os,
                                  const ExprBase<ExprT> &expr);

private:
  virtual const Symbol &Id() const = 0;
  virtual const llvm::ArrayRef<ExprT> SubExprs() const = 0;
  virtual void Format(llvm::raw_ostream &stream, llvm::StringRef style) const;
  friend struct llvm::format_provider<ExprBase<ExprT>>;
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

  bool operator!=(const Call<ExprT> &rhs) const { return !(*this == rhs); }

  friend std::ostream &operator<<(std::ostream &os, const Call<ExprT> &expr);

  const ExprT &target() const { return exprs_[0]; }

  const llvm::ArrayRef<ExprT> args() const {
    return llvm::ArrayRef(exprs_).slice(1);
  }

private:
  const Symbol &Id() const override { return Symbol::Get("call"); }

  const llvm::ArrayRef<ExprT> SubExprs() const override { return exprs_; }

  void Format(llvm::raw_ostream &stream, llvm::StringRef style) const override;

  std::vector<ExprT> exprs_;

  friend struct llvm::format_provider<Call<ExprT>>;
};

template <typename ExprT = Expression> class If final : public ExprBase<ExprT> {
public:
  If() : exprs_(std::make_unique<std::array<ExprT, 3>>()) {}

  If(ExprT cond, ExprT then, ExprT els)
      : exprs_(std::make_unique<std::array<ExprT, 3>>()) {
    (*exprs_)[0] = std::move(cond);
    (*exprs_)[1] = std::move(then);
    (*exprs_)[2] = std::move(els);
  }

  // Moveable
  // If(If&&) = default;
  // If& operator=(If&&) = default;

  bool operator==(const If<ExprT> &rhs) const {
    return cond() == rhs.cond() && then() == rhs.then() && els() == rhs.els();
  }

  bool operator!=(const If<ExprT> &rhs) const { return !(*this == rhs); }

  const ExprT &cond() const { return (*exprs_)[0]; }

  const ExprT &then() const { return (*exprs_)[1]; }

  const ExprT &els() const { return (*exprs_)[2]; }

private:
  const Symbol &Id() const override { return Symbol::Get("if"); }

  const llvm::ArrayRef<ExprT> SubExprs() const override { return *exprs_; }

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
  bool operator!=(const Return<ExprT> &rhs) const { return !(*this == rhs); }

  bool HasValue() const { return bool(value_); }

  const Expression &Value() const { return *value_; }

private:
  const Symbol &Id() const override { return Symbol::Get("return"); }

  const llvm::ArrayRef<ExprT> SubExprs() const override {
    if (value_) {
      return *value_;
    } else {
      return llvm::ArrayRef<ExprT>();
    }
  }

  const ExprT value_;
};

/*
  (type foo (Fn [i64 i64] i64))
  (fn foo [x y]
  (+ (x y))
*/
template <typename ExprT = Expression> class Fn final {
public:
  Fn() : params_(std::vector<const Symbol *>()), body_(std::vector<ExprT>()) {}
  Fn(const Symbol &name, std::vector<const Symbol *> &params,
     std::vector<ExprT> body)
      : name_(&name), params_(std::move(params)), body_(std::move(body)) {}

  // Moveable
  // Fn(Fn&&) = default;
  // Fn& operator=(Fn&&) = default;

  bool operator==(const Fn<ExprT> &rhs) const {
    return name_ == rhs.name_ && params_ == rhs.params_ && body_ == rhs.body_;
  }

  bool operator!=(const Fn<ExprT> &rhs) const { return !(*this == rhs); }

  friend std::ostream &operator<<(std::ostream &os, const Fn<ExprT> &fn);

  void SetName(const Symbol &name) { name_ = &name; }

  const Symbol *name() const { return name_; }

  void SetParams(std::vector<const Symbol *> params) {
    params_ = std::move(params);
  }

  llvm::ArrayRef<const Symbol *const> params() const { return params_; }

  void SetBody(std::vector<ExprT> &body) { body_ = std::move(body); }

  llvm::ArrayRef<ExprT> body() const { return body_; }

private:
  const Symbol *name_;
  std::vector<const Symbol *> params_;
  std::vector<ExprT> body_;
};

template <typename ExprT = Expression> class Array final {
public:
  Array(std::vector<ExprT> &&exprs) : exprs_(std::move(exprs)) {}

  bool operator==(const Array<ExprT> &rhs) const {
    return this->exprs_ == rhs.exprs_;
  }

  bool operator!=(const Array<ExprT> &rhs) const { return !(*this == rhs); }

  friend std::ostream &operator<<(std::ostream &os, const Array<ExprT> &expr);

  llvm::ArrayRef<ExprT> exprs() const { return exprs_; }

private:
  std::vector<ExprT> exprs_;
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
  friend std::ostream &operator<<(std::ostream &os, const VarDef &var);

  const Symbol &name() const { return *name_; }

  const TypeExpr &type() const { return type_; }

  const ExprT &init() const { return *init_; }
  bool HasInit() const { return init_ != nullptr; }

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
  friend std::ostream &operator<<(std::ostream &os, const MemberAccess &access);

  const ExprT &struct_expr() const { return *struct_expr_; }

  const Symbol &member() const { return *member_; }

private:
  std::unique_ptr<ExprT> struct_expr_;
  const Symbol *member_;
};

using Assignable = std::variant<const Symbol *, MemberAccess<Expression>>;

template <typename ExprT> class Set final {
public:
  Set() {}
  Set(Assignable place, std::unique_ptr<ExprT> value)
      : place_(std::move(place)), value_(std::move(value)) {}

  bool operator==(const Set &rhs) const {
    return place_ == rhs.place_ && *value_ == *rhs.value_;
  }

  bool operator!=(const Set &rhs) const { return !(*this == rhs); }
  friend std::ostream &operator<<(std::ostream &os, const Set &expr);

  const Assignable &place() const { return place_; }

  const ExprT &value() const { return *value_; }

private:
  Assignable place_;
  std::unique_ptr<ExprT> value_;
};

} // namespace exprs

class NameDecl final {
public:
  NameDecl() : name_(&Symbol::Get("")), type_(Symbol::Get("")) {}
  NameDecl(const Symbol &name, TypeExpr &type)
      : name_(&name), type_(std::move(type)) {}

  bool operator==(const NameDecl &rhs) const {
    return name_ == rhs.name_ && type_ == rhs.type_;
  }

  bool operator!=(const NameDecl &rhs) const { return !(*this == rhs); }

  friend std::ostream &operator<<(std::ostream &os, const NameDecl &def);

  const Symbol &name() const { return *name_; }

  const TypeExpr &type() const { return type_; }

private:
  const Symbol *name_;
  TypeExpr type_;
};

struct ModuleDecl final {
  const Symbol *name;

  // Constructs an "unnamed" module
  explicit ModuleDecl() {}

  explicit ModuleDecl(const Symbol &name) : name(&name) {}

  // Move only
  // ModuleDecl(ModuleDecl&&) = default;
  // ModuleDecl& operator=(ModuleDecl&&) = default;

  bool operator==(const ModuleDecl &rhs) const { return name == rhs.name; }

  bool operator!=(const ModuleDecl &rhs) const { return !(*this == rhs); }
  friend std::ostream &operator<<(std::ostream &os, const ModuleDecl &header);

  void SetName(const Symbol &name) noexcept { this->name = &name; }
};

using Call = exprs::Call<Expression>;
using If = exprs::If<Expression>;
using Fn = exprs::Fn<Expression>;
using Return = exprs::Return<Expression>;
using Array = exprs::Array<Expression>;
using VarDef = exprs::VarDef<Expression>;
using Set = exprs::Set<Expression>;
using MemberAccess = exprs::MemberAccess<Expression>;

using Assignable = exprs::Assignable;
template <typename... Ts> struct AssignableVisitor : Ts... {
  using Ts::operator()...;
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

llvm::ArrayRef<const Symbol *const> Symbols();

} // namespace special

// TODO: Rename to Node
// Expression represents an AST node.
class Expression final {
public:
  using Kind = std::variant<compiler::Constant, const Symbol *, Call, If, Fn,
                            Return, Array, VarDef, Set, MemberAccess,
                            ModuleDecl, TypeDef, NameDecl>;

  Expression() : kind_(compiler::Constant(Nil::Get())) {}

  // Copyable
  Expression(const Expression &) = delete;
  Expression &operator=(const Expression &) = delete;

  // Movable
  Expression(Expression &&) = default;
  Expression &operator=(Expression &&) = default;

  explicit Expression(Kind kind) : kind_(std::move(kind)) {}

  static Expression Literal(compiler::Constant x) { return Expression(x); }

  static Expression Name(const Symbol *x) { return Expression(x); }

  bool operator==(const Expression &rhs) const {
    return this->kind_ == rhs.kind_;
  }

  bool operator!=(const Expression &rhs) const { return !(*this == rhs); }

  friend std::ostream &operator<<(std::ostream &os,
                                  const Expression &expression);

  Kind &kind() { return kind_; }

  template <typename T> bool Is() const noexcept {
    return std::holds_alternative<T>(kind_);
  }

  template <typename T> T Get() const { return std::get<T>(kind_); }

  template <typename T> T *GetIf() { return std::get_if<T>(kind_); }

  template <typename Matcher> constexpr auto Match(Matcher &&f) const {
    return std::visit(f, kind_);
  }

  template <typename Matcher> constexpr auto MutableMatch(Matcher &&f) {
    return std::visit(f, kind_);
  }

  std::optional<Assignable> AsAssignable() {
    return std::visit(
        [&]<typename T>(T &&v) -> std::optional<Assignable> {
          // TODO: get types from Assignable
          if constexpr (std::disjunction_v<
                            std::is_same<std::decay_t<T>, const Symbol *>,
                            std::is_same<std::decay_t<T>, MemberAccess>>)
            return std::move(v);
          else
            return std::nullopt;
        },
        kind_);
  }

private:
  Kind kind_;
};

} // namespace dub

#endif /* DUB_EXPRESSION_H_ */
