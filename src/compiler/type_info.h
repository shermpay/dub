#ifndef DUB_COMPILER_TYPE_INFO_H_
#define DUB_COMPILER_TYPE_INFO_H_

#include "absl/container/node_hash_map.h"

#include "src/compiler/type.h"
#include "src/symbol.h"

namespace dub::compiler {

/// TypeInfo stores all type definitions, and types of names.
///
/// See individual methods for more details.
class TypeInfo final {
 public:
  /// LookupType returns the Type object from the name. 
  const std::optional<Type> LookupType(const Symbol& name) const;

  const std::optional<const type::Constructor*> LookupConstructor(const Symbol& name) const;

  /// LookupName returns the Type of the given name.
  const std::optional<Type> LookupName(const Symbol& name) const;

  void AddType(const Symbol& name, const Type type) {
    types_.insert({&name, type});
  };

  void AddConstructor(const Symbol& name, const type::Constructor* ctor) {
    ctors_.insert({&name, ctor});
  };

  void AddName(const Symbol& name, const Type type) {
    names_.insert({&name, type});
  }

  // TODO: Topological ordering
  auto types_cbegin() const {
    return types_.cbegin();
  }
  auto types_cend() const {
    return types_.cend();
  }

 private:
  absl::node_hash_map<const Symbol*, Type> types_;
  absl::node_hash_map<const Symbol*, const type::Constructor*> ctors_;
  absl::node_hash_map<const Symbol*, Type> names_;
};

}  // namespace dub::compiler

#endif /* DUB_COMPILER_TYPE_INFO_H_ */
