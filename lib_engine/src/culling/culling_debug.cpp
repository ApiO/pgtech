#include "culling_debug.h"
#include "geometry/geometric_system.h"

#include <runtime/idlut.h>

namespace pge
{
  namespace culling_debug
  {
    static bool display = false;

    void show(bool value)
    {
      display = value;
    }

    void gather(CullingSystem &s)
    {
      if (!display)
        return;

      // affichage des aabb
      Color color(255,0,0,255);
      glm::vec3 aabb[4];
      CullingSystem::Entry *e, *end = idlut::end(s);

      for (e = idlut::begin(s); e < end; e++) {
        aabb[0] = e->value.data[0];
        aabb[1] = glm::vec3(e->value.data[1].x, e->value.data[0].y, e->value.data[0].z);
        aabb[2] = e->value.data[1];
        aabb[3] = glm::vec3(e->value.data[0].x, e->value.data[1].y, e->value.data[0].z);
        geometric_system::gather_polygon(aabb, 4, color);
      }
    }
  }
}