#include <cstdint>
struct FreeNode {
  FreeNode *next;
};

struct ChunkHeader {
  uint32_t chunk_size;
  uint32_t payload_offset;
  uint32_t flags;
};
