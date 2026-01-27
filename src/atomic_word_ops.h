#include <cstdint>
static inline std::uint64_t atomic_word_load(std::uint64_t *p) {
  return __atomic_load_n(p, __ATOMIC_ACQUIRE);
}

static inline std::uint64_t atomic_word_fetch_and(std::uint64_t *p,
                                                  std::uint64_t mask) {
  return __atomic_fetch_and(p, mask, __ATOMIC_ACQ_REL);
}

static inline std::uint64_t atomic_word_fetch_or(std::uint64_t *p,
                                                 std::uint64_t mask) {
  return __atomic_fetch_or(p, mask, __ATOMIC_ACQ_REL);
}
