#include <cstdint>
static inline std::uint64_t atomic_word_load(uint64_t *p) {
  return __atomic_load_n(p, __ATOMIC_ACQUIRE);
}
