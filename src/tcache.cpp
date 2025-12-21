#include <cstddef>
#include <cstdint>
#include <forward_list>
class Tcache {
private:
  static constexpr uint8_t MAX_CLASSES{18};
  std::forward_list<int> buckets[MAX_CLASSES];
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
};
