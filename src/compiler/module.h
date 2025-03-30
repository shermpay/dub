#ifndef DUB_COMPILER_MODULE_H_
#define DUB_COMPILER_MODULE_H_

#include <memory>
#include <ostream>
#include <vector>

#include "absl/strings/str_format.h"
#include "absl/types/span.h"

#include "src/compiler/expression.h"
#include "src/compiler/expr_info.h"

namespace dub {

// A Module contains the imports and a sequence of Expressions, it is the root AST node.
// It owns the memory of all the children nodes.
class Module final {
 public:
  using ExprInfoOwner = std::unique_ptr<compiler::ExprInfo>;
  using ExprInfoList = std::vector<ExprInfoOwner>;

  explicit Module()  {}
  explicit Module(std::unique_ptr<ModuleDecl> header) :
      header_(std::move(header))
  {}

  Module(std::unique_ptr<ModuleDecl> header,
         std::vector<Expression> contents,
         ExprInfoList expr_infos) :
      header_(std::move(header)),
      contents_(std::move(contents)),
      expr_infos_(std::move(expr_infos))
       {}

  static Module WithName(const Symbol& name) {
    return Module(std::make_unique<ModuleDecl>(name));
  }

  const ModuleDecl& Header() const {
    return *header_;
  }

  absl::Span<const Expression> Contents() const {
    return contents_;
  }

  absl::Span<const ExprInfoOwner> Infos() const {
    return expr_infos_;
  }

  const compiler::ExprInfo& ExprInfoAt(std::uint64_t idx) const {
    return *expr_infos_.at(idx);
  }

  const compiler::ExprInfo* GetExprInfo(ExprId id) const {
    return &this->ExprInfoAt(id.ToKey());
  }

  const compiler::ExprInfo* ExprGetInfo(const Expression& expr) const {
    return GetExprInfo(expr.Id());
  }

  void SetHeader(ModuleDecl& header) {
    this->header_ = std::make_unique<ModuleDecl>(std::move(header));
  }

  Expression MakeExpression(Expression::Kind kind,
                             Module::ExprInfoOwner info);

  Expression& AddExpression(Expression expr) {
    this->contents_.push_back(std::move(expr));
    return this->contents_.back();
  }

  const ExprId NextId() const noexcept {
    return ExprId {
      .module_name = header_->name,
      .id = this->expr_infos_.size()};
  }

 private:
  friend std::ostream& operator<<(std::ostream& os, const Module& mod) {
    os << absl::StreamFormat("%v", mod);
    return os;
  }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Module& mod) {
    absl::Format(&sink, "%v\n", *mod.header_);
    for (const auto& expr : mod.contents_) {
      absl::Format(&sink, "%v\n", expr);
    }
  }

  std::unique_ptr<ModuleDecl> header_;
  std::vector<Expression> contents_;
  ExprInfoList expr_infos_;
};

}  // namespace dub

#endif /* DUB_COMPILER_MODULE_H_ */
