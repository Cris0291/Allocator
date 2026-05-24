#pragma once

#include <cstddef>

namespace alloc {
void *alloc(std::size_t size);
void free(void *raw);

template <typename T> T *alloc() { return static_cast<T *>(alloc(sizeof(T))); }
template <typename T> void free(T *t) { free(static_cast<void *>(t)); }
} // namespace alloc
