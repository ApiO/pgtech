#pragma once

#include <glm/glm.hpp>
#include <runtime/idlut.h>

#include "culling_types.h"

//**********************
// Frustum colling : Geometric approach
// source: http://www.lighthouse3d.com/tutorials/view-frustum-culling/
//**********************


namespace pge
{
  inline CullingSystem::CullingSystem(Allocator &a) : IdLookupTable<AABB>(a) {}
  inline CullingSystem::~CullingSystem(){}

  namespace culling_system
  {
    u64  create_aabb(CullingSystem &sys, const glm::vec2 *res);
    void destroy_aabb(CullingSystem &sys, u64 aabb);
    void update(CullingSystem &sys, u64 aabb, const glm::mat4 &world_pose, const glm::vec2 *res);
    void update(CullingSystem &sys, u64 aabb, const glm::mat4 &world_pose);
    void cull(CullingSystem &sys, const Frustum &frustum);
    bool visible(CullingSystem &sys, u64 aabb);
  }

  namespace _internal_culling_system
  {
    // point's signed distance from plane
    // source: http://www.lighthouse3d.com/tutorials/maths/plane/
    inline pge::f32 plane_dot_coord(const Plane &plane, const glm::vec3 &point)
    {
      return plane.a * point.x + plane.b * point.y + plane.c * point.z + plane.d;
    }

    inline bool sphere_in_frustum(const Plane *frustum, const glm::vec3 &pos, pge::f32 radius)
    {
      for (int i = 0; i < 6; i++) {
        if (plane_dot_coord(frustum[i], pos) + radius < 0)
          return false;
      }
      return true;
    }

    inline bool point_in_frustum(const Plane *frustum, const glm::vec3 &point)
    {
      for (pge::u32 i = 0; i < 6; i++) {
        if (plane_dot_coord(frustum[i], point) < 0.f)
          return false;
      }
      return true;
    }

    inline glm::vec3 vert_p(const Plane &frustum, const glm::vec3 *aabb)
    {
      glm::vec3 r = glm::vec3(aabb[0]);

      if (frustum.a > 0)
        r.x = aabb[1].x;
      if (frustum.b > 0)
        r.y = aabb[1].y;
      if (frustum.c > 0)
        r.z = aabb[1].z;

      return r;
    }

    inline glm::vec3 vert_n(const Plane &frustum, const glm::vec3 *aabb)
    {
      glm::vec3 r = glm::vec3(aabb[1]);

      if (frustum.a > 0)
        r.x = aabb[0].x;
      if (frustum.b > 0)
        r.y = aabb[0].y;
      if (frustum.c > 0)
        r.z = aabb[0].z;

      return r;
    }

    inline bool aabb_in_frustum(const Plane *frustum, const glm::vec3 *aabb)
    {
      // http://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-testing-boxes-ii/

      for (pge::u32 i = 0; i < 6; i++) {
        // is the positive vertex outside?
        f32 tmp = plane_dot_coord(frustum[i], vert_p(frustum[i], aabb));
        if (tmp < 0)
          return false; //OUTSIDE;
        // is the negative vertex outside?
        //else if (plane_dot_coord(frustum[i], vert_n(frustum[i], aabb)) < 0)
        //INTERSECT;
      }
      return true;
    }
  }

  namespace culling_system
  {
    inline u64 create_aabb(CullingSystem &sys, const glm::vec2 *res)
    {
      AABB aabb;
      memcpy(aabb.res, res, sizeof(glm::vec2) * 2);
      return idlut::add(sys, aabb);
    }

    inline void destroy_aabb(CullingSystem &sys, u64 id)
    {
      idlut::remove(sys, id);
    }

    inline void update(CullingSystem &sys, u64 id, const glm::mat4 &world_pose, const glm::vec2 *res)
    {
      AABB &aabb = *idlut::lookup(sys, id);
      memcpy(aabb.res, res, sizeof(glm::vec2) * 2);
      update(sys, id, world_pose);
    }

    inline void update(CullingSystem &sys, u64 id, const glm::mat4 &world_pose)
    {
      const glm::vec4 origin(0, 0, 0, 1);
      AABB &aabb = *idlut::lookup(sys, id);

      glm::vec4 min = glm::vec4(aabb.res[0], 0, 1);
      glm::vec4 max = glm::vec4(aabb.res[1], 0, 1);

      glm::vec4 verts[6];
      verts[0] = glm::vec4(max.x, min.y, min.z, 1);
      verts[1] = glm::vec4(max.x, max.y, min.z, 1);
      verts[2] = glm::vec4(min.x, max.y, min.z, 1);

      verts[3] = glm::vec4(min.x, max.y, max.z, 1);
      verts[4] = glm::vec4(min.x, min.y, max.z, 1);
      verts[5] = glm::vec4(max.x, min.y, max.z, 1);

      min = world_pose * min;
      max = world_pose * max;

      for (i32 i = 0; i < 6; i++) {
        verts[i] = world_pose * verts[i];

        if (verts[i].x < min.x)
          min.x = verts[i].x;
        else if (verts[i].x > max.x)
          max.x = verts[i].x;

        if (verts[i].y < min.y)
          min.y = verts[i].y;
        else if (verts[i].y > max.y)
          max.y = verts[i].y;

        if (verts[i].z < min.z)
          min.z = verts[i].z;
        else if (verts[i].z > max.z)
          max.z = verts[i].z;
      }

      aabb.data[0] = glm::vec3(min);
      aabb.data[1] = glm::vec3(max);
    }

    inline void cull(CullingSystem &sys, const Frustum &frustum)
    {
      using namespace _internal_culling_system;
      IdLookupTable<AABB>::Entry *aabb, *end = idlut::end(sys);

      for (aabb = idlut::begin(sys); aabb < end; aabb++)
        aabb->value.visible = aabb_in_frustum((Plane*)&frustum, aabb->value.data);
    }

    inline bool visible(CullingSystem &sys, u64 id)
    {
      return idlut::lookup(sys, id)->visible;
    }
  }
}