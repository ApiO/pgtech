#pragma once

#include <runtime/types.h>

namespace pge
{
  struct PhysicsResource
  {
    struct Material
    {
      u32 name;
      f32 density;
      f32 friction;
      f32 restitution;
    };

    struct CollisionFilter
    {
      u32 name;
      u16 is;
      u16 collides_with;
    };

    struct Actor
    {
      u32 name;
      u8  dynamic;
      u8  kinematic;
      u16 disable_gravity;
    };

    struct Shape
    {
      u32 name;
      u32 trigger;
      u32 collision_filter;
    };
    i32 ppm;
    u32 num_materials;
    u32 num_collision_filters;
    u32 num_shapes;
    u32 num_actors;
    
    u32 _material_offset;
    u32 _collision_filter_offset;
    u32 _shape_offset;
    u32 _actor_offset;
  };
}