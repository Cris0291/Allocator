#ifndef ALLOC_API_H
#define ALLOC_API_H

#include <cstddef>

namespace alloc {

T *Alloc<T>(size_t size);
void Free<T>(T *pointer);
} // namespace alloc

#endif
