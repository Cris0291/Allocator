#include "alloc_api.h"
#include "tcache.h"

TCache tcache{};

void *alloc::alloc(std::size_t size) { tcache.allocate(size); };

void alloc::free(void *raw) { tcache.free(raw); };
