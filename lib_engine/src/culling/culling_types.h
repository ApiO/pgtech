#pragma once

#include <runtime/collection_types.h>
#include <glm/glm.hpp>

namespace pge
{
  struct AABB
  {
    glm::vec2 res[2];  // resource manager stored vertices 
    glm::vec3 data[2]; // aabb updated with world_pose (not normalized)
    bool visible;
  };

#pragma warning(push)
  struct Plane
  {
#pragma warning( disable:4201 )
    union
    {
      struct { glm::vec4 data; };
      struct { pge::f32 a, b, c, d; };
    }; 
#pragma warning(pop)
  };

  struct Frustum
  {
    Plane left;
    Plane right;
    Plane top;
    Plane bottom;
    Plane _near;
    Plane _far;
  };

  struct CullingSystem : IdLookupTable<AABB>
  {
    CullingSystem(Allocator &a);
    ~CullingSystem();
  };
}