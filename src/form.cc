#include "src/form.h"

#include "src/form_format.h" // IWYU pragma: keep

#include "llvm/Support/FormatVariadic.h"

#include <ostream>

namespace dub {

template <>
List::List(MutableList &mut_list)
    : values_(
          std::make_shared<form::ListContainer>(std::move(mut_list.Data()))),
      head_idx_(0) {}

std::ostream &operator<<(std::ostream &os, const Form &form) {
  os << llvm::formatv("{0}", form).str();
  return os;
}

namespace form {

template <> std::ostream &operator<<(std::ostream &os, const List<Form> &list) {
  os << llvm::formatv("{0}", list).str();
  return os;
}

} // namespace form

// The following has to be outside the struct definition, otherwise it triggers
// a bug in gcc (works in clang)
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85282
template <> const Symbol &Form::get<const Symbol &>() const {
  return *std::get<const Symbol *>(value);
}

template <> List List::Empty() {
  static List list(std::make_shared<ListContainer>(), 0);
  return list;
}

} // namespace dub
