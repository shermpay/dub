#ifndef DUB_COMPILER_PARSER_H_
#define DUB_COMPILER_PARSER_H_

#include "src/compiler/expression.h"
#include "src/compiler/module.h"
#include "src/compiler/result.h"
#include "src/errors.h"
#include "src/form.h"

#include "llvm/Support/Error.h"

#include <system_error>

namespace dub::compiler {

class Parser;

namespace parser {

class Error : public llvm::ErrorInfo<Error> {
public:
  static char ID;
  explicit Error(std::string msg) : message_(msg) {}

  void log(llvm::raw_ostream &os) const override { os << message_; }

  std::error_code convertToErrorCode() const override {
    return make_error_code(ErrorCode::kParserError);
  }

  friend std::ostream &operator<<(std::ostream &os, const Error &error) {
    os << error.message_;
    return os;
  }

private:
  // TODO: Add location
  std::string message_;
};

} // namespace parser

// Parsing forms into expressions and special forms.
class Parser final {
public:
  explicit Parser() {}

  llvm::Expected<Module> ParseModule(const List &forms) const;

  llvm::Expected<ModuleDecl> ParseModuleHeader(const Form &form,
                                               Module *modul) const;

  // Parses a top-level expression and add it to the Module.
  llvm::Expected<Expression *> ParseExpression(const Form &form,
                                               Module *modul) const;

private:
  // Parsing Expression Visitor
  class ParseAsExpr final {
  public:
    explicit ParseAsExpr(const dub::compiler::Parser *parser, const Form &form,
                         Module *modul)
        : parser_(parser), form_(form), modul_(modul) {}
    Result<Expression> operator()(const Constant::Literal &) const;
    Result<Expression> operator()(const Symbol *) const;
    Result<Expression> operator()(const List &) const;
    Result<Expression> operator()(const Vector &) const;

  private:
    const Parser *parser_;
    const Form &form_;
    Module *modul_;
  };
  class ParseAsType final {
  public:
    explicit ParseAsType(const dub::compiler::Parser *parser, const Form &form,
                         Module *modul)
        : parser_(parser),
          // form_(form),
          modul_(modul) {
      (void)form;
    }
    Result<TypeExpr> operator()(const Form::Value &) const;
    Result<TypeExpr> operator()(const List &) const;
    Result<TypeExpr> operator()(const Vector &) const;

  private:
    Result<TypeExpr::Struct> ParseStruct(const List &, Module *modul) const;

    const Parser *parser_;
    // const Form& form_;
    Module *modul_;
  };
  Result<Expression> ParseExpr(const Form &form, Module *modul) const;
  Result<If> ParseIf(const List &list, Module *modul) const;
  Result<Fn> ParseFn(const List &list, Module *modul) const;
  Result<Call> ParseCall(const List &list, Module *modul) const;
  Result<Return> ParseReturn(const List &list, Module *modul) const;
  Result<VarDef> ParseVarDef(const List &list, Module *modul) const;
  Result<Set> ParseVarSet(const List &list, Module *modul) const;
  Result<MemberAccess> ParseMemberAccess(const List &list, Module *modul) const;
  Result<ModuleDecl> ParseModuleDecl(const List &list, Module *modul) const;
  Result<TypeDef> ParseTypeDef(const List &list, Module *modul) const;
  Result<NameDecl> ParseNameDecl(const List &list, Module *modul) const;
  Result<TypeExpr> ParseType(const Form &form, Module *modul) const;
};

} // namespace dub::compiler
#endif /* DUB_COMPILER_PARSER_H_ */
