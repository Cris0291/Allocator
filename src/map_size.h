#include <cstddef>

struct MapSizeAlignment {
  std::size_t size;
  std::size_t alignment;
};

constexpr MapSizeAlignment map_info[] = {
    {8, 16},   {16, 16},  {24, 16},  {32, 16},  {40, 16},  {48, 16},
    {64, 16},  {80, 16},  {96, 16},  {128, 16}, {160, 16}, {192, 16},
    {256, 16}, {320, 16}, {384, 16}, {448, 16}, {512, 64},
};

constexpr std::size_t NUM_CLASSES = sizeof(map_info) / sizeof(map_info[0]);
