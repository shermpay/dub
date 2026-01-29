#ifndef DUB_COMPILER_MODULE_H_
#define DUB_COMPILER_MODULE_H_

#include <memory>
#include <ostream>
#include <vector>

#include "src/compiler/expr_info.h"
#include "src/compiler/expression.h"

namespace dub {

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
};

} // namespace dub

template <> struct llvm::DenseMapInfo<dub::ExprId> {
  static inline dub::ExprId getEmptyKey() { return dub::ExprId::Default(); }
  static inline dub::ExprId getTombstoneKey() {
    return dub::ExprId{nullptr, static_cast<std::uint64_t>(-1)};
  }
  static unsigned getHashValue(const dub::ExprId &val) { return val.ToKey(); }

  static bool isEqual(const dub::ExprId &LHS, const dub::ExprId &RHS) {
    return LHS == RHS;
  }
};

namespace dub {

// A Module contains the imports and a sequence of Expressions, it is the root
// AST node. It owns the memory of all the children nodes.
class Module final {
public:
  using ExprInfoOwner = std::unique_ptr<compiler::ExprInfo>;
  using ExprInfoList = std::vector<ExprInfoOwner>;

  explicit Module() {}
  explicit Module(std::unique_ptr<ModuleDecl> header)
      : header_(std::move(header)) {}

  Module(std::unique_ptr<ModuleDecl> header, std::vector<Expression> contents,
         ExprInfoList expr_infos)
      : header_(std::move(header)), contents_(std::move(contents)),
        expr_infos_(std::move(expr_infos)) {}

  static Module WithName(const Symbol &name) {
    return Module(std::make_unique<ModuleDecl>(name));
  }

  const ModuleDecl &Header() const { return *header_; }

  llvm::ArrayRef<Expression> Contents() const { return contents_; }

  llvm::ArrayRef<ExprInfoOwner> Infos() const { return expr_infos_; }

  const compiler::ExprInfo &ExprInfoAt(std::uint64_t idx) const {
    return *expr_infos_.at(idx);
  }

  const compiler::ExprInfo *GetExprInfo(ExprId id) const {
    return &this->ExprInfoAt(id.ToKey());
  }

  const compiler::ExprInfo *ExprGetInfo(const Expression &expr) const {
    return GetExprInfo(expr.Id());
  }

  void SetHeader(ModuleDecl &header) {
    this->header_ = std::make_unique<ModuleDecl>(std::move(header));
  }

  Expression MakeExpression(Expression::Kind kind, Module::ExprInfoOwner info);

  Expression &AddExpression(Expression expr) {
    this->contents_.push_back(std::move(expr));
    return this->contents_.back();
  }

  const ExprId NextId() const noexcept {
    return ExprId{.module_name = header_->name, .id = this->expr_infos_.size()};
  }

private:
  friend std::ostream &operator<<(std::ostream &os, const Module &mod);

  std::unique_ptr<ModuleDecl> header_;
  std::vector<Expression> contents_;
  ExprInfoList expr_infos_;
};

} // namespace dub

namespace llvm {
template <> struct format_provider<dub::Module> {
  static void format(const dub::Module &mod, raw_ostream &stream,
                     StringRef style);
};
} // namespace llvm

#endif /* DUB_COMPILER_MODULE_H_ */
