#include "application.h"
#include <runtime/temp_allocator.h>
#include <runtime/idlut.h>
#include <runtime/hash.h>
#include "resource/resource_manager.h"
#include "physics/physics_system.h"
#include "scene/scene_system.h"
#include "text/text_system.h"
#include "sprite/sprite_system.h"
#include "animation/animation_player.h"
#include <runtime/murmur_hash.h>
#include <engine/matrix.h>

namespace pge
{
  namespace unit
  {
    void create_objects(World &w, u64 unit, const DecomposedMatrix &wp)
    {
      Unit &u = *idlut::lookup(w.units, unit);
      Allocator &a = *w.units._data._allocator;

      const u32 num_nodes  = unit_resource::num_nodes(u.resource);

      // init component numbers
      u.num_actors        = unit_resource::num_actors(u.resource);
      u.num_sprites       = unit_resource::num_sprites(u.resource);
      u.num_movers        = unit_resource::num_movers(u.resource);
      u.num_cameras       = 0; // TODO
      u.num_texts         = 0; // TODO

      // init component offsets
      u.actor_offset        = 0;
      u.sprite_offset       = u.actor_offset + u.num_actors;
      u.mover_offset        = u.sprite_offset + u.num_sprites;
      u.camera_offset       = 0; // TODO
      u.text_offset         = 0; // TODO

      // init animation data
      u.player_session = 0;
      u.animation_set = u.resource->animation_set ?
        (AnimsetResource*)resource_manager::get(RESOURCE_TYPE_ANIMSET, u.resource->animation_set) : 0;

      // allocate enough memory to store reference to all components we're gonna create
      u.components = (ComponentRef*)a.allocate(sizeof(ComponentRef)*(u.mover_offset + u.num_movers));

      const UnitResource::Node   *res_nodes   = unit_resource::nodes(u.resource);
      const UnitResource::Actor  *res_actors  = unit_resource::actors(u.resource);
      const UnitResource::Sprite *res_sprites = unit_resource::sprites(u.resource);
      const MoverResource        *res_movers  = unit_resource::movers(u.resource);

      const glm::vec3 zangle(0, 0, 1);

      { // create the unit scene graph
        TempAllocator512 ta(a);
        Array<DecomposedMatrix> poses(ta);
        Array<i32>  parents(ta);

        array::resize(poses, num_nodes);
        array::resize(parents, num_nodes);

        for (u32 i = 0; i < num_nodes; i++) {
          poses[i].translation = glm::vec3(res_nodes[i].pose.tx, res_nodes[i].pose.ty, 0.f);
          poses[i].rotation    = glm::quat(glm::vec3(res_nodes[i].pose.rx, res_nodes[i].pose.ry, res_nodes[i].pose.rz));
          poses[i].scale       = glm::vec3(res_nodes[i].pose.sx, res_nodes[i].pose.sy, 1.f);
          parents[i]           = res_nodes[i].parent;
        }

        poses[0] = wp;

        u.scene_graph = scene_system::add(w.scene_system, array::begin(poses), array::begin(parents), num_nodes);
        scene_system::update(w.scene_system, u.scene_graph);
      }

      ComponentRef *cr = u.components;
      UnitRef ur;
      DecomposedMatrix lp;
      ur.unit = unit;

      // create actors
      for (u32 i = 0; i < u.num_actors; i++) {
        lp.translation = glm::vec3(res_actors[i].pose.tx, res_actors[i].pose.ty, 0.f);
        lp.rotation    = glm::quat(glm::vec3(res_actors[i].pose.rx, res_actors[i].pose.ry, res_actors[i].pose.rz));
        lp.scale       = glm::vec3(res_actors[i].pose.sx, res_actors[i].pose.sy, 1.f);

        cr->component  = physics_system::create_actor(w.physics_world, *(ActorData*)resource_manager::get(RESOURCE_TYPE_ACTOR, res_actors[i].actor), unit & idlut_internal::INDEX_MASK);
        cr->node       = res_actors[i].node;
        ur.node        = cr->node;

        hash::set(w.actor_unit, cr->component, ur);
        Pose &p = physics_system::get_pose(w.physics_world, cr->component);
        pose::set_local_pose(p, lp);
        pose::transform_world_pose(p, scene_system::world_pose(w.scene_system, u.scene_graph, cr->node));

        cr++;
      }

      // create sprites
      for (u32 i = 0; i < u.num_sprites; i++) {
        lp.translation = glm::vec3(res_sprites[i].pose.tx, res_sprites[i].pose.ty, 0.f);
        lp.rotation    = glm::quat(glm::vec3(res_sprites[i].pose.rx, res_sprites[i].pose.ry, res_sprites[i].pose.rz));
        lp.scale       = glm::vec3(res_sprites[i].pose.sx, res_sprites[i].pose.sy, 1.f);

        cr->component = sprite_system::create(w.sprite_system, (SpriteResource*)resource_manager::get(RESOURCE_TYPE_SPRITE, res_sprites[i].tpl), unit & idlut_internal::INDEX_MASK, res_sprites[i].order);
        cr->node      = res_sprites[i].node;
        ur.node       = cr->node;

        hash::set(w.sprite_unit, cr->component, ur);
        pose::set_local_pose(sprite_system::get_pose(w.sprite_system, cr->component), lp);

        cr++;
      }

      // create movers
      for (u32 i = 0; i < u.num_movers; i++) {
        lp.translation = glm::vec3(res_movers[i].offset[0], res_movers[i].offset[1], 0.f);

        cr->component  = physics_system::create_mover(w.physics_world, &res_movers[i], unit & idlut_internal::INDEX_MASK);
        cr->node       = 0; // mover is always attached to the root node
        ur.node        = cr->node;

        hash::set(w.mover_unit, cr->component, ur);
        physics_system::teleport_mover(w.physics_world, cr->component, wp.translation + lp.translation);

        cr++;
      }
    }

    void destroy_objects(World &w, u64 unit)
    {
      Allocator &a = *w.units._data._allocator;
      Unit &u = *idlut::lookup(w.units, unit);

      if (u.player_session)
        animation_player::destroy_session(w.animation_player, u.player_session);

      u32 end = u.actor_offset + u.num_actors;
      for (u32 i = u.actor_offset; i < end; i++) {
        physics_system::destroy_actor(w.physics_world, u.components[i].component);
        hash::remove(w.actor_unit, u.components[i].component);
      }

      end = u.sprite_offset + u.num_sprites;
      for (u32 i = u.sprite_offset; i < end; i++) {
        sprite_system::destroy(w.sprite_system, u.components[i].component);
        hash::remove(w.sprite_unit, u.components[i].component);
      }

      end = u.mover_offset + u.num_movers;
      for (u32 i = u.mover_offset; i < end; i++) {
        physics_system::destroy_mover(w.physics_world, u.components[i].component);
        hash::remove(w.mover_unit, u.components[i].component);
      }

      a.deallocate(u.components);
    }
  }

  namespace unit
  {
    void play_animation(u64 world, u64 unit, const char *animation, f32 from, f32 to, bool loop, f32 speed)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);
      const u32 anim_name = murmur_hash_32(animation);

      if (u.player_session) {
        if (animation_player::playing_animation(w.animation_player, u.player_session) == anim_name)
          return;

        animation_player::destroy_session(w.animation_player, u.player_session);
      }

      u.player_session = animation_player::create_session(w.animation_player, u.animation_set);
      animation_player::play(w.animation_player, u.player_session, anim_name, from, to, loop, speed);
    }

    bool is_playing_animation(u64 world, u64 unit)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);
      return u.player_session != 0;
    }

    void get_world_position(u64 world, u64 unit, i32 node, glm::vec3 &v)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);

      v = scene_system::world_translation(w.scene_system, u.scene_graph, node);
    }

    void get_world_rotation(u64 world, u64 unit, i32 node, glm::quat &q)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);

      q = scene_system::world_rotation(w.scene_system, u.scene_graph, node);
    }

    void get_world_scale(u64 world, u64 unit, i32 node, glm::vec3 &v)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);

      v = scene_system::world_scale(w.scene_system, u.scene_graph, node);
    }

    void get_world_pose(u64 world, u64 unit, i32 node, glm::mat4 &m)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);

      m = scene_system::world_pose(w.scene_system, u.scene_graph, node);
    }

    void get_local_position(u64 world, u64 unit, i32 node, glm::vec3 &v)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);

      v = scene_system::local_translation(w.scene_system, u.scene_graph, node);
    }

    void get_local_rotation(u64 world, u64 unit, i32 node, glm::quat &q)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);

      q = scene_system::local_rotation(w.scene_system, u.scene_graph, node);
    }

    void get_local_scale(u64 world, u64 unit, i32 node, glm::vec3 &v)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);

      v = scene_system::local_scale(w.scene_system, u.scene_graph, node);
    }

    void get_local_pose(u64 world, u64 unit, i32 node, glm::mat4 &m)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);

      compose_mat4(m, scene_system::local_pose(w.scene_system, u.scene_graph, node));
    }

    void set_local_position(u64 world, u64 unit, i32 node, const glm::vec3 &translation)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);

      scene_system::set_local_translation(w.scene_system, u.scene_graph, node, translation);
      if (node == 0) u.mover = UINT32_MAX;
    }

    void set_local_rotation(u64 world, u64 unit, i32 node, const glm::quat &q)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);

      scene_system::set_local_rotation(w.scene_system, u.scene_graph, node, q);
      if (node == 0) u.mover = UINT32_MAX;
    }

    void set_local_scale(u64 world, u64 unit, i32 node, const glm::vec3 &s)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);

      scene_system::set_local_scale(w.scene_system, u.scene_graph, node, s);
      if (node == 0) u.mover = UINT32_MAX;
    }

    void set_local_pose(u64 world, u64 unit, i32 node, const glm::mat4 &m)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);

      scene_system::set_local_pose(w.scene_system, u.scene_graph, node, m);
      if (node == 0) u.mover = UINT32_MAX;
    }

    i32 actor_index(u64 world, u64 unit, const char *name)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);
      const UnitResource::Actor  *res_actors = unit_resource::actors(u.resource);
      const u32 hname = murmur_hash_32(name);

      for (u32 i = 0; i < u.num_actors; i++) {
        if (res_actors[i].instance_name == hname)
          return i;
      }
      return -1;
    }

    u64 actor(u64 world, u64 unit, i32 i)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);
      return (u.components + u.actor_offset + i)->component;
    }

    u64 actor(u64 world, u64 unit, const char *name)
    {
      const i32 i = actor_index(world, unit, name);
      XASSERT(i >= 0, "could not find the actor named \"%s\"", name);
      return actor(world, unit, i);
    }

    i32 mover_index(u64 world, u64 unit, const char *name)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);
      const MoverResource *res_movers  = unit_resource::movers(u.resource);
      const u32 hname = murmur_hash_32(name);

      for (u32 i = 0; i < u.num_movers; i++) {
        if (res_movers[i].name == hname)
          return i;
      }
      return -1;
    }

    u64 mover(u64 world, u64 unit, i32 i)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);
      return (u.components + u.mover_offset + i)->component;
    }

    u64 mover(u64 world, u64 unit, const char *name)
    {
      const i32 i = mover_index(world, unit, name);
      XASSERT(i >= 0, "could not find the mover named \"%s\"", name);
      return mover(world, unit, i);
    }

    i32 sprite_index(u64 world, u64 unit, const char *name)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);
      const UnitResource::Sprite *res_sprites  = unit_resource::sprites(u.resource);
      const u32 hname = murmur_hash_32(name);

      for (u32 i = 0; i < u.num_movers; i++) {
        if (res_sprites[i].name == hname)
          return i;
      }
      return -1;
    }

    u64 sprite(u64 world, u64 unit, i32 i)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);
      return (u.components + u.sprite_offset + i)->component;
    }

    u64 sprite(u64 world, u64 unit, const char *name)
    {
      const i32 i = sprite_index(world, unit, name);
      XASSERT(i >= 0, "could not find the sprite named \"%s\"", name);
      return sprite(world, unit, i);
    }

    const char *is_a(u64 world, u64 unit, const char *name)
    {
      const u32 h = murmur_hash_32(name);
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);

      if (!resource_manager::has_loaded(RESOURCE_TYPE_UNIT, h))
        return 0;

      return (UnitResource*)resource_manager::get(RESOURCE_TYPE_UNIT, h) == u.resource ? name : 0;
    }

    u32 name_hash(u64 world, u64 unit)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);
      const u32 h = resource_manager::name((void*)u.resource);
      XASSERT(h, "could not find the unit resource");
      return h;
    }

    inline static void merge_aabb(Box &box, AABB *aabb)
    {
      if (aabb->data[0].x < box.min.x)
        box.min.x = aabb->data[0].x;
      if (aabb->data[0].y < box.min.y)
        box.min.y = aabb->data[0].y;
      if (aabb->data[0].z < box.min.z)
        box.min.z = aabb->data[0].z;

      if (aabb->data[1].x > box.max.x)
        box.max.x = aabb->data[1].x;
      if (aabb->data[1].y > box.max.y)
        box.max.y = aabb->data[1].y;
      if (aabb->data[1].z > box.max.z)
        box.max.z = aabb->data[1].z;
    }

    void box(u64 world, u64 unit, Box &box)
    {
      World &w = application::world(world);
      Unit  &u = *idlut::lookup(w.units, unit);
      ComponentRef *cr;

      box.min = glm::vec3(FLT_MAX);
      box.max = glm::vec3(-FLT_MAX);

      cr = u.components + u.sprite_offset;
      for (u32 i = 0; i < u.num_sprites; i++)
        merge_aabb(box, idlut::lookup(w.culling_system, idlut::lookup(w.sprite_system.sprites, cr[i].component)->aabb));
    }
  }
}