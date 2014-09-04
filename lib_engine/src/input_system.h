#pragma once

#include <runtime/memory_types.h>

namespace pge
{
  namespace input_system
  {
    void init(void *win, Allocator &a);
    void update(void);
    void shutdown(void);
  }
}