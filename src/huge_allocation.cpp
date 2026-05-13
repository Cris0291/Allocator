#include "huge_allocation.h"
#include "os_api.h"
#include <cstddef>
#include <cstdint>
#include <new>

void *HugeAlloc::alloc(std::size_t size, std::size_t alignment) {
  os_api::MemSpan mem_span;
  os_api::OsResult res;
  res = os_api::reserve_address_space(size + sizeof(HugeHeader), alignment,
                                      mem_span);
  if (res != os_api::OsResult::Success)
    return nullptr;

  res = os_api::commit_memory(mem_span.addr, mem_span.size);

  if (res != os_api::OsResult::Success)
    return nullptr;

  void *raw{
      HugeAlloc::include_header(reinterpret_cast<std::uintptr_t>(mem_span.addr),
                                mem_span.size - sizeof(HugeHeader))};
  return raw;
};

void HugeAlloc::free(void *raw) {
  auto res{HugeAlloc::get_header_size(raw)};
  os_api::OsResult free_res{os_api::release_addresss_space(
      res.first, res.second + sizeof(HugeHeader))};
  if (free_res != os_api::OsResult::Success)
    throw std::bad_alloc{};
};
