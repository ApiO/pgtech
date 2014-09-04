#include <runtime/types.h>
#include <runtime/trace.h>

#include <data/physics.h>

#include "compiler_types.h"
#include "compiler.h"

namespace
{
  using namespace pge;

  const u16 BIT_MASK[] ={
    1u, 2u, 4u, 8u, 16u, 32u, 64u, 128u, 256u, 512u, 1024u, 2048u, 4096u,
    8192u, 16384u, 32768u
  };
}

namespace pge
{
  PhysicsCompiler::PhysicsCompiler(Allocator &a, StringPool &sp)
    : JsonCompiler(a, RESOURCE_TYPE_PHYSICS, sp) {}

  bool PhysicsCompiler::compile(Work &w)
  {
    if (!load_json(w)) return false;
    u64 node_id;

    PhysicsResource::Material        material;
    PhysicsResource::CollisionFilter filter;
    PhysicsResource::Actor           actor;
    PhysicsResource::Shape           shape;

    PhysicsResource res;
    res.ppm                   = json::get_node(jsn, json::get_id(jsn, json::root(jsn), "ppm")).value.integer;
    res.num_materials         = json::size(jsn, json::get_id(jsn, json::root(jsn), "materials"));
    res.num_collision_filters = json::size(jsn, json::get_id(jsn, json::root(jsn), "collision_filters"));
    res.num_shapes            = json::size(jsn, json::get_id(jsn, json::root(jsn), "shapes"));
    res.num_actors            = json::size(jsn, json::get_id(jsn, json::root(jsn), "actors"));

    res._material_offset         = sizeof(PhysicsResource);
    res._collision_filter_offset = res._material_offset + sizeof(PhysicsResource::Material)*res.num_materials;
    res._shape_offset            = res._collision_filter_offset + sizeof(PhysicsResource::CollisionFilter)*res.num_collision_filters;
    res._actor_offset            = res._shape_offset + sizeof(PhysicsResource::Shape)*res.num_shapes;

    // write header
    fwrite(&res, sizeof(PhysicsResource), 1, w.data);

    // writes materials
    node_id = json::get_id(jsn, json::root(jsn), "materials");
    for (u32 i = 0; i < res.num_materials; i++) {
      const Json::Node &node = json::get_node(jsn, node_id, i);

      material.name        = compiler::create_id_string(w, node.name);
      material.density     = (f32)json::get_number(jsn, node.id, "density");
      material.friction    = (f32)json::get_number(jsn, node.id, "friction");
      material.restitution = (f32)json::get_number(jsn, node.id, "restitution");

      fwrite(&material, sizeof(PhysicsResource::Material), 1, w.data);
    }

    // write collision filters
    {
      // create a hash storing a bit mask for the given collision type name
      Hash<u16> collision_type_masks(*a);
      node_id = json::get_id(jsn, json::root(jsn), "collision_types");
      const u32 num_collision_types = json::size(jsn, node_id);
      for (u32 i = 0; i < num_collision_types; i++) {
        const u32 collision_type_name = compiler::create_id_string(w, json::get_string(jsn, node_id, i));
        hash::set(collision_type_masks, collision_type_name, BIT_MASK[i]);
      }

      // writes collision filters
      node_id = json::get_id(jsn, json::root(jsn), "collision_filters");
      for (u32 i = 0; i < res.num_collision_filters; i++) {
        const Json::Node &node = json::get_node(jsn, node_id, i);

        filter.name = compiler::create_id_string(w, node.name);
        filter.is = filter.collides_with = 0;

        // set is bits
        u64 next = json::get_node(jsn, node.id, "is").child;
        while (next != json::NO_NODE) {
          const Json::Node &child_node = json::get_node(jsn, next);
          const u32 collision_type = compiler::create_id_string(w, json::get_node(jsn, next).value.string);

          if (!hash::has(collision_type_masks, collision_type)) {
            LOG("Could not find the collision type named \"%s\"", json::get_node(jsn, next).value.string);
            return false;
          }

          filter.is |= hash::get(collision_type_masks, collision_type, (u16)0);
          next = child_node.next;
        }

        // set collides_with bits
        next = json::get_node(jsn, node.id, "collides_with").child;
        while (next != json::NO_NODE) {
          const Json::Node &child_node = json::get_node(jsn, next);
          const u32 collision_type = compiler::create_id_string(w, json::get_node(jsn, next).value.string);

          if (!hash::has(collision_type_masks, collision_type)) {
            LOG("Could not find the collision type named \"%s\"", json::get_node(jsn, next).value.string);
            return false;
          }

          filter.collides_with |= hash::get(collision_type_masks, collision_type, (u16)0);
          next = child_node.next;
        }

        fwrite(&filter, sizeof(PhysicsResource::CollisionFilter), 1, w.data);
      }
    }

    // write shapes
    node_id = json::get_id(jsn, json::root(jsn), "shapes");
    for (u32 i = 0; i < res.num_shapes; i++) {
      const Json::Node &node = json::get_node(jsn, node_id, i);

      shape.name = compiler::create_id_string(w, node.name);
      shape.collision_filter = compiler::create_id_string(w, json::get_string(jsn, node.id, "collision_filter"));
      shape.trigger = json::get_bool(jsn, node.id, "trigger", false);

      fwrite(&shape, sizeof(PhysicsResource::Shape), 1, w.data);
    }

    // writes actors
    node_id = json::get_id(jsn, json::root(jsn), "actors");
    for (u32 i = 0; i < res.num_actors; i++) {
      const Json::Node &node = json::get_node(jsn, node_id, i);

      actor.name            = compiler::create_id_string(w, node.name);
      actor.dynamic         = json::get_bool(jsn, node.id, "dynamic") ? 1 : 0;
      actor.kinematic       = json::get_bool(jsn, node.id, "kinematic") ? 1 : 0;
      actor.disable_gravity = json::get_bool(jsn, node.id, "disable_gravity", false) ? 1 : 0;

      fwrite(&actor, sizeof(PhysicsResource::Actor), 1, w.data);
    }

    return true;
  }
}