#pragma once

#include "arena.h"
#include <cstddef>

namespace alloc {

T *Alloc<T>(size_t size);
void Free<T>(T *pointer);
} // namespace alloc
