#ifndef DUB_COMPILER_PARSER_H_
#define DUB_COMPILER_PARSER_H_

#include <optional>

#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"

#include "src/compiler/expression.h"
#include "src/compiler/module.h"
#include "src/form.h"

namespace dub::compiler {

class Parser;

namespace parser {

class Error {
 public:
  explicit Error(std::string msg) : message_(msg) {}

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Error& error) {
    sink.Append(error.message_);
  }

  friend std::ostream& operator<<(std::ostream& os, const Error& error) {
    os << error.message_;
    return os;
  }
 private:
  // TODO: Add location
  std::string message_;
};

template <typename T>
class Result {
 public:
  // Copy contructor. For copyable values.
  Result(const Result&) = default;
  // Move constructor
  Result(Result&&) = default;

  Result(const T& value) : value_(value), error_(std::optional<parser::Error>()) {}
  Result(const T& value, std::optional<Error> err) : value_(std::move(value)), error_(err) {}

  Result(T&& value) : value_(std::move(value)), error_(std::optional<parser::Error>()) {}
  Result(T&& value, std::optional<Error> err) : value_(std::move(value)), error_(err) {}

  static Result Error(const T& value, std::string message) {
    return Result(value, std::optional<parser::Error>(message));
  }

  static Result Error(T&& value, std::string message) {
    return Result(std::move(value), std::optional<parser::Error>(message));
  }

  template <typename... Args>
  static Result Errorf(const T& value, absl::FormatSpec<Args...> fmt, const Args&... args) {
    return Result(value, std::optional<parser::Error>(absl::StrFormat(fmt, args...)));
  }

  template <typename... Args>
  static Result Errorf(T&& value, absl::FormatSpec<Args...> fmt, const Args&... args) {
    return Result(std::move(value), std::optional<parser::Error>(absl::StrFormat(fmt, args...)));
  }

  static Result FromError(T&& value, parser::Error error) {
    return Result(std::move(value), std::optional<parser::Error>(error));
  }

  bool IsError() const {
    return error_.has_value();
  }

  bool IsOk() const {
    return !IsError();
  }

  const parser::Error& Error() const& {
    return *error_;
  }

  parser::Error Error() && {
    return *error_;
  }

  T& Value() & {
    return value_;
  }

  const T& Value() const& {
    return std::move(value_);
  }

  T&& Value() && {
    return std::move(value_);
  }

  const T&& Value() const&& {
    return std::move(value_);
  }

  template <typename T2>
  Result<T2> UpdateValue(T2&& new_value) {
    return parser::Result<T2>(std::move(new_value), error_);
  }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Result<T>& result) {
    if (result.IsError()) {
      AbslStringify(sink, result.Error());
    } else {
      absl::Format(&sink, "%v", result.Value());
    }
  }

  friend std::ostream& operator<<(std::ostream& os, const Result<T>& result) {
    os << absl::StreamFormat("%v", result);
    return os;
  }

 private:
  T value_;
  std::optional<parser::Error> error_;
};


}  // namespace parser

// Parsing forms into expressions and special forms.
class Parser final {
 public:
  explicit Parser() {}

  absl::StatusOr<Module> ParseModule(const List& forms) const;

  absl::StatusOr<ModuleDecl> ParseModuleHeader(const Form& form, Module* modul) const;

  // Parses a top-level expression and add it to the Module.
  absl::StatusOr<Expression*> ParseExpression(const Form& form, Module* modul) const;
 private:
  // Parsing Expression Visitor
  class ParseAsExpr final {
   public:
    explicit ParseAsExpr(const dub::compiler::Parser* parser,
                         const Form& form,
                         Module* modul) :
        parser_(parser),
        form_(form),
        modul_(modul)
    {}
    parser::Result<Expression> operator()(const Constant::Literal&) const;
    parser::Result<Expression> operator()(const Symbol*) const;
    parser::Result<Expression> operator()(const List&) const;
    parser::Result<Expression> operator()(const Vector&) const;
   private:
    const Parser* parser_;
    const Form& form_;
    Module* modul_;
  };
  class ParseAsType final {
   public:
    explicit ParseAsType(
        const dub::compiler::Parser* parser,
        const Form& form,
        Module* modul) :
        parser_(parser),
        // form_(form),
        modul_(modul)
    { (void)form; }
    parser::Result<TypeExpr> operator()(const Form::Value&) const;
    parser::Result<TypeExpr> operator()(const List&) const;
    parser::Result<TypeExpr> operator()(const Vector&) const;
   private:
    parser::Result<TypeExpr::Struct> ParseStruct(const List&, Module* modul) const;

    const Parser* parser_;
    // const Form& form_;
    Module* modul_;
  };
  parser::Result<Expression> ParseExpr(const Form& form, Module* modul) const;
  parser::Result<If> ParseIf(const List& list, Module* modul) const;
  parser::Result<Fn> ParseFn(const List& list, Module* modul) const;
  parser::Result<Call> ParseCall(const List& list, Module* modul) const;
  parser::Result<Return> ParseReturn(const List& list, Module* modul) const;
  parser::Result<VarDef> ParseVarDef(const List& list, Module* modul) const;
  parser::Result<Set> ParseVarSet(const List& list, Module* modul) const;
  parser::Result<MemberAccess> ParseMemberAccess(const List& list, Module* modul) const;
  parser::Result<ModuleDecl> ParseModuleDecl(const List& list, Module* modul) const;
  parser::Result<TypeDef> ParseTypeDef(const List& list, Module* modul) const;
  parser::Result<NameDecl> ParseNameDecl(const List& list, Module* modul) const;
  parser::Result<TypeExpr> ParseType(const Form& form, Module* modul) const;
};


}  // namespace dub::compiler
#endif /* DUB_COMPILER_PARSER_H_ */
