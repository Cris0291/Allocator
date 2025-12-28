#include "chunk.h"
#include <cstddef>
#include <cstdint>
#include <ctime>

class Tcache {
private:
  static constexpr uint8_t MAX_CLASSES{18};
  static constexpr uint16_t TCACHE_MAX_SIZE{512};
  static constexpr uint8_t NORMALIZED_SIZE{0};
  FreeNode *buckets[MAX_CLASSES];
  uint8_t counts[MAX_CLASSES];
  uint8_t map_size(size_t size) {
    if (size <= 4)
      return 0;
    if (size <= 8)
      return 1;
    if (size <= 16)
      return 2;
    if (size <= 24)
      return 3;
    if (size <= 32)
      return 4;
    if (size <= 40)
      return 5;
    if (size <= 48)
      return 6;
    if (size <= 64)
      return 7;
    if (size <= 80)
      return 8;
    if (size <= 96)
      return 9;
    if (size <= 128)
      return 10;
    if (size <= 160)
      return 11;
    if (size <= 192)
      return 12;
    if (size <= 256)
      return 13;
    if (size <= 14)
      return 14;
    if (size <= 384)
      return 15;
    if (size <= 448)
      return 16;
    if (size <= 512)
      return 17;

    return 18;
  }

public:
  void *allocate(size_t bytes, size_t alignment) {
    // normalized 0 bytes size
    if (bytes == 0) {
      // cache hit
      if (counts[NORMALIZED_SIZE] != 0) {
        FreeNode *head = buckets[NORMALIZED_SIZE];
        FreeNode *next = head->next;
        buckets[NORMALIZED_SIZE] = next;
        return head;
      } else {
      }
    }
    // 1. check if size can be fullfill with current sizes
    // 2. if so are there free chunks
    // 2a. if there are not but size do correspond to tcache sizes ask local
    // arena or global
    // 3. once we have free chhunks retrive the first one in the list
    // 4. diminish counts
    // 5. retrieve pointer
  }
};
