#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>

struct MapSizeAlignment {
  std::size_t size;
  std::size_t alignment;
};

struct SizeAlignmentResult {
  bool has_size{};
  std::optional<uint8_t> idx;
};

constexpr MapSizeAlignment map_info[] = {
    {8, 16},   {16, 16},  {24, 16},  {32, 16},  {40, 16},  {48, 16},
    {64, 16},  {80, 16},  {96, 16},  {128, 16}, {160, 16}, {192, 16},
    {256, 16}, {320, 16}, {384, 16}, {448, 16}, {512, 64},
};

constexpr std::size_t NUM_CLASSES = std::size(map_info);
constexpr std::size_t ALLOC_MIN_ALIGNMENT{alignof(std::max_align_t)};
constexpr uint8_t NO_CLASS{0xff};
