#include "src/source_info.h"

#include <string>

#include "absl/container/node_hash_set.h"

namespace dub {

absl::node_hash_set<std::string>& FilenamesPool() {
  static absl::node_hash_set<std::string> pool;
  return pool;
};

std::string FormDebugString(const Form& form) noexcept {
  return absl::StrFormat("%v => %v", form, *form.info);
}

}  // namespace dub
