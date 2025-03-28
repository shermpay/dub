#include "type_info.h"

#include "absl/log/die_if_null.h"

namespace dub::compiler {

const std::optional<Type> TypeInfo::LookupType(const Symbol& name) const {
  auto name_ptr = ABSL_DIE_IF_NULL(&name);
  auto iter = types_.find(name_ptr);

  if (iter == types_.end()) {
    return std::nullopt;
  }
  return iter->second;
}

const std::optional<const type::Constructor*> TypeInfo::LookupConstructor(
    const Symbol& name) const {
  auto iter = ctors_.find(&name);

  if (iter == ctors_.end()) {
    return std::nullopt;
  }
  return iter->second;
}

const std::optional<Type> TypeInfo::LookupName(const Symbol& name) const {
  auto iter = names_.find(&name);

  if (iter == names_.end()) {
    return std::nullopt;
  }
  return iter->second;
}

}  // namespace dub::compiler
