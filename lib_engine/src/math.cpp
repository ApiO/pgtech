#include "engine/pge_types.h"

namespace pge
{
  namespace math
  {
    // https://truesculpt.googlecode.com/hg-history/Release%25200.8/Doc/ray_box_intersect.pdf
    float ray_box_intersection(const glm::vec3 &from, const glm::vec3 &dir, const Box &box)
    {
      float tmin, tmax, tymin, tymax, tzmin, tzmax;
      if (dir.x >= 0) {
        tmin = (box.min.x - from.x) / dir.x;
        tmax = (box.max.x - from.x) / dir.x;
      } else {
        tmin = (box.max.x - from.x) / dir.x;
        tmax = (box.min.x - from.x) / dir.x;
      }
      if (dir.y >= 0) {
        tymin = (box.min.y - from.y) / dir.y;
        tymax = (box.max.y - from.y) / dir.y;
      } else {
        tymin = (box.max.y - from.y) / dir.y;
        tymax = (box.min.y - from.y) / dir.y;
      }
      if ((tmin > tymax) || (tymin > tmax))
        return -1;
      if (tymin > tmin)
        tmin = tymin;
      if (tymax < tmax)
        tmax = tymax;
      if (dir.z >= 0) {
        tzmin = (box.min.z - from.z) / dir.z;
        tzmax = (box.max.z - from.z) / dir.z;
      } else {
        tzmin = (box.max.z - from.z) / dir.z;
        tzmax = (box.min.z - from.z) / dir.z;
      }
      if ((tmin > tzmax) || (tzmin > tmax))
        return -1;
      /*
      // t0 & t1 represent a valid interval
      if (tzmin > tmin)
        tmin = tzmin;
      if (tzmax < tmax)
        tmax = tzmax;
      return ((tmin < t1) && (tmax > t0));*/

      if (tzmin > tmin)
        tmin = tzmin;
      if (tzmax < tmax)
        tmax = tzmax;

      return tmin > 0 ? tmin : tmax;
    }
  }
}