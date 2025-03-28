#include "src/form.h"

#include <cstdint>
#include <ostream>
#include <variant>

#include "absl/strings/str_format.h"

namespace dub {


template<>
List::List(MutableList& mut_list) :
    values_(std::make_shared<form::ListContainer>(std::move(mut_list.Data()))),
    head_idx_(0)
{};


std::ostream& operator<<(std::ostream& os, const Form& form) {
  os << absl::StreamFormat("%v", form);
  return os;
};


namespace form {

std::ostream& operator<<(std::ostream& os, const List<Form>& list) {
  os << absl::StreamFormat("%v", list);
  return os;
};

}  // namespace form

// The following has to be outside the struct definition, otherwise it triggers a bug in gcc (works in clang)
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85282
template <>
const Symbol& Form::get<const Symbol&>() const {
  return *std::get<const Symbol*>(value);
};


Form NilValue() {
  static Form form(Nil::Get());
  return form;
};

template<>
List List::Empty() {
  static List list(std::make_shared<ListContainer>(), 0);
  return list;
};

}  // namespace dub
