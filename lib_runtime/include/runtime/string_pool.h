#pragma once

#include "runtime/collection_types.h"

namespace pge
{
  namespace string_pool
  {
    char *acquire (StringPool &pool, const char *str, u32 len);
    char *acquire (StringPool &pool, const char *str);
    void  release (StringPool &pool, const char *str);
    bool  has     (StringPool &pool, const char *str);
  }
}