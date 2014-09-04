#pragma once

#include <data/physics.h>

namespace pge
{
  namespace physics_resource
  {
    i32 get_ppm(const PhysicsResource *res);
    u32 num_actors(const PhysicsResource *res);
    u32 num_materials(const PhysicsResource *res);
    u32 num_shapes(const PhysicsResource *res);
    u32 num_collision_filters(const PhysicsResource *res);

    const PhysicsResource::Actor    *actors(const PhysicsResource *res);
    const PhysicsResource::Material *materials(const PhysicsResource *res);
    const PhysicsResource::Shape    *shapes(const PhysicsResource *res);
    const PhysicsResource::CollisionFilter *collision_filters(const PhysicsResource *res);
  }

  namespace physics_resource
  {
    inline i32 get_ppm(const PhysicsResource *res)
    {
      return res->ppm;
    }

    inline u32 num_actors(const PhysicsResource *res)
    {
      return res->num_actors;
    }

    inline u32 num_shapes(const PhysicsResource *res)
    {
      return res->num_shapes;
    }

    inline u32 num_materials(const PhysicsResource *res)
    {
      return res->num_materials;
    }

    inline u32 num_collision_filters(const PhysicsResource *res)
    {
      return res->num_collision_filters;
    }

    inline const PhysicsResource::Actor *actors(const PhysicsResource *res)
    {
      return (const PhysicsResource::Actor*)(((const u8*)res) + res->_actor_offset);
    }

    inline const PhysicsResource::Shape *shapes(const PhysicsResource *res)
    {
      return (const PhysicsResource::Shape*)(((const u8*)res) + res->_shape_offset);
    }

    inline const PhysicsResource::Material *materials(const PhysicsResource *res)
    {
      return (const PhysicsResource::Material*)(((const u8*)res) + res->_material_offset);
    }

    inline const PhysicsResource::CollisionFilter *collision_filters(const PhysicsResource *res)
    {
      return (const PhysicsResource::CollisionFilter*)(((const u8*)res) + res->_collision_filter_offset);
    }
  }
}