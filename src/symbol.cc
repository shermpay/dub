#include "symbol.h"

namespace dub {

const Symbol& Symbol::Get(std::string name) {
  // TODO: This is not thread safe, as Interns is not a concurrent map
  auto& interns = Interns();
  auto iter = interns.find(name);
  if (iter == interns.end()) {
    auto [new_iter, inserted] = interns.try_emplace(name, name, CtorKey{});
    return new_iter->second;
  }
  return iter->second;
}

std::ostream& operator<<(std::ostream& os, const Symbol& symbol) {
  os << symbol.value();
  return os;
}

}  // namespace dub
