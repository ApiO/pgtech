#pragma once

#include <glm/glm.hpp>

#include <runtime/types.h>
#include <runtime/memory_types.h>
#include <runtime/collection_types.h>
#include <scene/scene_types.h>

#include <engine/pge_types.h>
#include <data/shape.h>
#include <data/physics.h>
#include <data/mover.h>
#include <pose.h>

namespace pge
{
  typedef void Space;
  typedef void Body;
  typedef void Shape;
  typedef void Fixture;
  
  struct ActorTemplate
  {
    bool kinematic;
    bool dynamic;
    bool gravity;
  };

  struct MaterialTemplate
  {
    f32 density;
    f32 friction;
    f32 restitution;
  };

  struct ShapeTemplate
  {
    u32 trigger;
    u32 collision_filter;
  };

  struct CollisionFilter
  {
    u16 is;
    u16 collides_with;
  };

  struct ShapeData
  {
    ShapeType type;
    u32  num_components;
    f32 *components;
  };

  struct ActorData
  {
    struct Shape
    {
      u32 _instance_name; // TODO : existance douteuse !
      MaterialTemplate *material;
      ShapeTemplate    *tpl;
      ShapeData        *shape;
      DecomposedMatrix  pose;
    };
    ActorTemplate *actor;
    u32    num_shapes;
    Shape *shapes;
  };

  struct ShapeInfo
  {
    ShapeType type;
    Fixture  *fixture;
    u64       actor;
  };

  struct Actor
  {
    Pose      pose;
    Body     *body;
    bool      dynamic;
    bool      kinematic;
    bool      gravity;
    bool      old_flip;
    glm::vec3 old_scale;
  };

  struct ActorListener
  {
    struct PrevContact
    {
      bool was_touching;
      ContactPoint contact;
    };

    void(*touched_callback)   (const Array<ContactPoint> &contacts);
    void(*untouched_callback) (const Array<ContactPoint> &actors);
    Array<ContactPoint> *result;
    Array<ContactPoint> *contacts;
    Hash<PrevContact>   *prev_contacts;
  };

  struct Raycast
  {
    RaycastCallback callback;
    bool closest;
    bool any;
    u16  is;
    u16  collides_with;
    Array<ContactPoint> *hits;
  };

  struct Mover
  {
    u64  capsule;
    bool collides_down;
    bool collides_up;
    bool collides_left;
    bool collides_right;
    bool _dirty;
    glm::vec3 _dp;
    MoverResource *res;
  };
  
  struct PhysicsWorld
  {
    PhysicsWorld(f64 step_time, Allocator &a);
    ~PhysicsWorld();

    f64 step_time, accumulator;

    Space *space;
    IdLookupTable<Actor>   actors;
    Hash<Shape*>           actor_shapes;
    Hash<u64>              body_actor;
    Hash<ShapeInfo>        shape_infos;
    IdLookupTable<Mover>   movers;
    IdLookupTable<Raycast> raycasts;
    Hash<ActorListener>    listeners;
    bool                   simulate;
  };
}