#include "super_block.h"

std::size_t SuperBlock::bitmap_size_convergence_routine(std::size_t header_sz) {
  std::size_t N0 = std::floor(span_size - header_sz) / SLOT_SIZE;
  double N1{};
  while (1) {
    std::size_t bitmap_words{N0 / 64};
    std::size_t bitmap_size{8 * bitmap_words};
    N1 = std::floor((span_size - header_sz - bitmap_size)) / SLOT_SIZE;
    if (N0 == N1)
      break;
    N0 = N1;
  }
  return N0;
}
std::uintptr_t align_up(std::uintptr_t x, std::size_t size) {
  return (x + (size - 1)) & ~(size - 1);
}

SuperBlock::SuperBlock(uint32_t class_id, ExtentManager &extent_manager,
                       std::size_t slot_size)
    : manager(extent_manager) {
  SLOT_SIZE = slot_size;
  std::size_t super_block_header_sz{sizeof(SuperBlockHeader)};
  std::size_t header_aligned_sz{align_up(super_block_header_sz, 16)};
  // calulate payload size
  std::size_t slots{bitmap_size_convergence_routine(header_aligned_sz)};
  total_number_slots = slots;
  std::size_t bitmap_sz{(slots / 64) * 8};
  std::uintptr_t payload_align_sz{align_up(header_aligned_sz + bitmap_sz, 16)};

  void *raw{manager.alloc_extent(span_size)};
  if (!raw)
    throw std::bad_alloc{};

  raw_base = raw;
  std::uintptr_t base{reinterpret_cast<uintptr_t>(raw)};
  SuperBlockHeader *header{reinterpret_cast<SuperBlockHeader *>(base)};
  super_block_header = header;
  header->class_id = class_id;
  header->span_size = span_size;
  header->slot_size = SLOT_SIZE;
  header->n_slot = slots;
  header->free_count = slots;
  header->payload_ptr = base + payload_align_sz;

  std::uintptr_t bitmap_ptr{base + header_aligned_sz};
  bitmap = reinterpret_cast<std::uint64_t *>(bitmap_ptr);
  // initialiize bitmap to 0's
  void *bitmap_vptr{reinterpret_cast<void *>(bitmap_ptr)};
  std::memset(bitmap_vptr, 0, bitmap_sz);
  // remove the unused bits in the last word
  std::size_t used_bits{header->n_slot % 64};
  if (used_bits) {
    std::size_t last_word_index{header->n_slot / 64};
    std::uint64_t mask{~((1ULL << used_bits) - 1)};
    bitmap[last_word_index] = mask;
  }
}

void *SuperBlock::allocate_atomic_span(std::size_t hint_word = 0) {
  std::size_t bitmap_words{total_number_slots / 64};
  if (hint_word > bitmap_words)
    hint_word = 0;

  while (hint_word <= bitmap_words) {
    std::uint64_t old_word = atomic_word_load(&bitmap[hint_word]);
    std::uint64_t free_mask = ~old_word;
    if (free_mask == 0) {
      ++hint_word;
      continue;
    }
    int bit = __builtin_ctzll(free_mask);
    std::uint64_t single_mask = 1ULL << bit;

    std::uint64_t prev = atomic_word_fetch_or(&bitmap[hint_word], single_mask);
    if ((prev & single_mask) == 0) {
      super_block_header->free_count.fetch_sub(1);
      // TODO check if span is empty
      std::size_t slot_index = hint_word * 64 + bit;
      std::uintptr_t payload =
          super_block_header->payload_ptr + slot_index * SLOT_SIZE;
      // How should i return a hint also
      return reinterpret_cast<void *>(payload);
    }
  }
  // if reached this is full
  return nullptr;
}

void SuperBlock::free_atomic_span(void *payload) {
  std::uintptr_t p{reinterpret_cast<std::uintptr_t>(payload)};
  std::uintptr_t offset{p - super_block_header->payload_ptr};
  // TODO 0 <= offset < N * SLOT_SIZE also offset % SLOT_SIZE
  std::uintptr_t slot_index{offset / SLOT_SIZE};
  std::uintptr_t word_index{slot_index / 64};
  std::uintptr_t bit{slot_index % 64};
  std::uint64_t single_mask = 1ULL << bit;
  std::uint64_t prev{atomic_word_fetch_and(&bitmap[word_index], ~single_mask)};
  // if((prev & single_mask) == 0) double free corruption hanlde it
  // TODO handle over addtion surpasing the available amount of slots
  super_block_header->free_count.fetch_add(1);
  return;
}

bool SuperBlock::is_full() {
  return super_block_header->free_count.load(std::memory_order_acquire) == 0;
}

bool SuperBlock::is_empty() {
  return super_block_header->free_count.load(std::memory_order_acquire) ==
         super_block_header->n_slot;
}

uint32_t SuperBlock::free_count() {
  return super_block_header->free_count.load(std::memory_order_acquire);
}

void SuperBlock::release() { manager.free_extent(raw_base); }

bool SuperBlock::is_range(std::uintptr_t ptr) {
  std::uintptr_t base{reinterpret_cast<std::uintptr_t>(raw_base)};
  return base <= ptr && ptr < (base + span_size);
}
