#pragma once

#include <engine/matrix_types.h>
#include "application_types.h"

namespace pge
{
  namespace unit
  {
    void create_objects  (World &w, u64 unit, const DecomposedMatrix &wp);
    void destroy_objects (World &w, u64 unit);
  }
}