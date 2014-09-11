#include <runtime/idlut.h>
#include <runtime/tree.h>
#include <runtime/array.h>
#include <runtime/murmur_hash.h>
#include <runtime/temp_allocator.h>
#include <runtime/trace.h>
#include <runtime/timer.h>
#include <runtime/hash.h>
#include <runtime/array.h>

#include <data/unit.h>

#include <engine/matrix.h>
#include "application.h"
//#include "culling/culling_system.h"
#include "resource/resource_manager.h"
#include "resource/level_resource.h"
#include "animation/animation_player.h"
#include "geometry/geometric_system.h"
#include "scene/scene_system.h"
#include "physics/physics_system.h"
#include "sprite/sprite_system.h"
#include "particle/particle_system.h"
#include "text/text_system.h"
#include "unit.h"
#include "camera/camera_system.h"

#if CHRONO_STEPS
#include <utils/app_watcher.h>
#endif

namespace
{
  using namespace pge;

  typedef u64(*Spawner) (u64 world, u32 name, const glm::vec3 &translation, const glm::quat &rotation, const glm::vec3 &scale);
  
  static u64 _spawn_unit(u64 world, u32 name, const glm::vec3 &translation, const glm::quat &rotation, const glm::vec3 &scale)
  {
    World &w = application::world(world);
    Unit u;
    u64  id;
    DecomposedMatrix p;

    u.resource = (UnitResource*)resource_manager::get(RESOURCE_TYPE_UNIT, name);
    u.mover    = UINT32_MAX;
    id = idlut::add(w.units, u);

    p.translation = translation;
    p.rotation    = rotation;
    p.scale       = scale;

    unit::create_objects(w, id, p);

    return id;
  }
  
  static u64 _spawn_sprite(u64 world, u32 name, const glm::vec3 &translation, const glm::quat &rotation, const glm::vec3 &scale)
  {
    World &w = application::world(world);
    u64   id = sprite_system::create(w.sprite_system, (SpriteResource*)resource_manager::get(RESOURCE_TYPE_SPRITE, name), 0, 0);
    Pose  &p = sprite_system::get_pose(w.sprite_system, id);

    pose::set_local_translation(p, translation);
    pose::set_local_rotation(p, rotation);
    pose::set_local_scale(p, scale);

    return id;
  }
  

  void _despawn_unit(World &w, u64 unit)
  {
    if(!idlut::has(w.units, unit)) return;

    unit::destroy_objects(w, unit);
    idlut::remove(w.units, unit);
  }

  void _unload_level(World &w, u64 level)
  {
    ASSERT(idlut::has(w.levels, level));

    Level *lv  = *idlut::lookup(w.levels, level);

    //despawn units
    for (u32 i = 0; i < array::size(lv->units); i++) {
      if (idlut::has(w.units, lv->units[i]))
        _despawn_unit(w, lv->units[i]);
    }

    //despawn sprites
    for (u32 i = 0; i < array::size(lv->sprites); i++)
      sprite_system::destroy(w.sprite_system, lv->sprites[i]);

    MAKE_DELETE((*app->_a), Level, lv);
    idlut::remove(w.levels, level);
  }

  u64 _create_actor(PhysicsWorld &w, ShapeType type,
                    const f32 *components, u32 num_components,
                    bool kinematic, bool dynamic, bool gravity, bool trigger,
                    const char *collision_filter, const char *material)
  {
    ActorTemplate actor_template;
    actor_template.kinematic = kinematic;
    actor_template.dynamic   = dynamic;
    actor_template.gravity   = gravity;

    ActorData::Shape actor_shape;
    actor_shape.material = (MaterialTemplate*)&physics_system::get_material_template(murmur_hash_32(material));

    ShapeTemplate shape_template;
    shape_template.trigger = trigger;
    shape_template.collision_filter = murmur_hash_32(collision_filter);
    actor_shape.tpl = &shape_template;

    TempAllocator1024 ta;
    actor_shape.shape = (ShapeData*)ta.allocate(sizeof(ShapeData)+num_components * sizeof(f32));
    actor_shape.shape->type = type;
    actor_shape.shape->num_components = num_components;

    f32 *data = (f32*)(((u8*)actor_shape.shape) + sizeof(ShapeData));
    memcpy(data, components, num_components * sizeof(f32));
    actor_shape.shape->components = data;

    ActorData actor_data;
    actor_data.actor      = &actor_template;
    actor_data.num_shapes = 1;
    actor_data.shapes     = &actor_shape;

    u64 id = physics_system::create_actor(w, actor_data);

    ta.deallocate(actor_data.shapes[0].shape->components);

    return id;
  }

  void load_level_resource(Array<u64> &container, u32 num_res, const u64 &world, const LevelResource::Resource *res, const glm::mat4 &mlevel, Spawner func)
  {
    glm::mat4 munit;
    DecomposedMatrix dm;

    for (u32 i = 0; i < num_res; i++) {
      const PoseResource &pose = res[i].pose;

      dm.translation.x = pose.tx;
      dm.translation.y = pose.ty;
      dm.translation.z = pose.tz;

      dm.rotation = glm::quat(glm::vec3(pose.rx, pose.ry, pose.rz));
      dm.scale = glm::vec3(pose.sx, pose.sy, 1.f);
      
      compose_mat4(munit, dm);

      munit = mlevel * munit;
      decompose_mat4(munit, dm);

      array::push_back(container, func(world, res[i].name, dm.translation, dm.rotation, dm.scale));
    }
  }
}

namespace pge
{
  namespace world
  {
    f64 delta_time(u64 world)
    {
      return (*idlut::lookup(app->worlds, world))->delta_time;
    }

    f64 total_time(u64 world)
    {
      return (*idlut::lookup(app->worlds, world))->total_time;
    }

    void update_anims(u64 world, f64 delta_time)
    {
      World &w = **idlut::lookup(app->worlds, world);
      IdLookupTable<Unit>::Entry *e, *end = idlut::end(w.units);
      DecomposedMatrix dm;

#if CHRONO_STEPS
      Timer(timer);
      start_timer(timer);
#endif
      animation_player::update(w.animation_player, delta_time);
#if CHRONO_STEPS
      app_watcher::save_value(WL_ANIMATION_UPDATE, get_elapsed_time_in_ms(timer));
      start_timer(timer);
#endif

      for (e = idlut::begin(w.units); e < end; e++) {
        Unit &u = e->value;

        if (!u.player_session)
          continue;

        // release played session
        if (animation_player::played(w.animation_player, u.player_session)) {
          animation_player::destroy_session(w.animation_player, u.player_session);
          u.player_session = 0;
          continue;
        }

        // extract poses from the session into the scene graph
        const UnitResource::Node *nodes = unit_resource::nodes(u.resource);
        const UnitResource::BoneTrack *bone_tracks = unit_resource::bone_tracks(u.resource);
        const UnitResource::SpriteTrack *sprite_tracks = unit_resource::sprite_tracks(u.resource);

        for (u16 i = 0; i < u.resource->num_bone_tracks; i++) {
          glm::vec2 t(nodes[bone_tracks[i].node].pose.tx, nodes[bone_tracks[i].node].pose.ty);
          glm::vec2 s(nodes[bone_tracks[i].node].pose.sx, nodes[bone_tracks[i].node].pose.sy);
          glm::vec3 r(nodes[bone_tracks[i].node].pose.rx, nodes[bone_tracks[i].node].pose.ry, nodes[bone_tracks[i].node].pose.rz);

          t   += animation_player::get_bone_track_translation(w.animation_player, e->value.player_session, bone_tracks[i].track);
          r.z += glm::radians(animation_player::get_bone_track_rotation(w.animation_player, e->value.player_session, bone_tracks[i].track));
          s   += animation_player::get_bone_track_scale(w.animation_player, e->value.player_session, bone_tracks[i].track) - glm::vec2(1, 1);

          dm.translation = glm::vec3(t.x, t.y, 0);
          dm.rotation    = glm::quat(r);
          dm.scale       = glm::vec3(s.x, s.y, 1);
          scene_system::set_local_pose(w.scene_system, u.scene_graph, bone_tracks[i].node, dm);
        }

        for (u16 i = 0; i < u.resource->num_sprite_tracks; i++) {
          const u16 *sprite_track_frames = unit_resource::sprite_track_frames(u.resource, sprite_tracks + i);
          const u32 frame = animation_player::get_sprite_track_frame(w.animation_player, e->value.player_session, sprite_tracks[i].track);

          if (frame != DEFAULT_FRAME && frame != NO_FRAME) {
            const ComponentRef *cr = u.components + u.sprite_offset + sprite_tracks[i].sprite;
            sprite_system::set_frame(w.sprite_system, cr->component, sprite_track_frames[frame]);
          }
        }

        //events handling
      }
#if CHRONO_STEPS
      app_watcher::save_value(WL_ANIMATION_TO_SCENE, get_elapsed_time_in_ms(timer));
#endif
    }

    void update_scene(u64 world, f64 delta_time)
    {
#if CHRONO_STEPS
      Timer(frame_timer);
      Timer(tmp_timer);

      start_timer(frame_timer);
      start_timer(tmp_timer);
#endif
      // ---------------------------------------------------------------
      // Update Timers
      // ---------------------------------------------------------------
      World &w = **idlut::lookup(app->worlds, world);
      w.delta_time   = delta_time;
      w.total_time  += delta_time;

      // ---------------------------------------------------------------
      // Update Physics
      // ---------------------------------------------------------------
      physics_system::update(w.physics_world, delta_time);

#if CHRONO_STEPS
      stop_timer(tmp_timer);
      app_watcher::save_value(WL_PHYSICS_UPDATES, get_elapsed_time_in_ms(tmp_timer));
      start_timer(tmp_timer);
#endif

      DecomposedMatrix dm;
      glm::mat4 m, tmp;
      glm::vec3 p;
      IdLookupTable<Unit>::Entry *e, *end = idlut::end(w.units);
      ComponentRef *cr;
      Unit         *u;

      // ---------------------------------------------------------------
      // Physics to Scene Graph
      // ---------------------------------------------------------------

      for (e = idlut::begin(w.units); e < end; e++) {
        u  = &e->value;

        // actors to scene graph
        cr = u->components + u->actor_offset;
        for (u32 i = 0; i < u->num_actors; i++) {
          if (!w.physics_world.simulate || !physics_system::is_physical(w.physics_world, cr[i].component))
            continue;
          const Pose &pose = physics_system::get_pose(w.physics_world, cr[i].component);
          pose::get_world_pose(pose, m);
          pose::get_local_pose(pose, dm);
          compose_mat4(tmp, dm);
          scene_system::set_world_pose(w.scene_system, u->scene_graph, cr[i].node, m / tmp);
        }

        // movers to scene_graph
        cr = u->components + u->mover_offset;
        if (u->mover != UINT32_MAX) {
          const MoverResource &mr = unit_resource::movers(u->resource)[u->mover];
          physics_system::get_mover_position(w.physics_world, cr[u->mover].component, p);
          scene_system::set_local_translation(w.scene_system, u->scene_graph, 0, p - glm::vec3(mr.offset[0], mr.offset[1], 0));
        }
      }

#if CHRONO_STEPS
      stop_timer(tmp_timer);
      app_watcher::save_value(WL_PHYSICS_TO_SCENE, get_elapsed_time_in_ms(tmp_timer));
      start_timer(tmp_timer);
#endif

      // ---------------------------------------------------------------
      // Scene graph update
      // ---------------------------------------------------------------
      scene_system::update(w.scene_system);

#if CHRONO_STEPS
      stop_timer(tmp_timer);
      app_watcher::save_value(WL_SCENE_SYSTEM_UPDATE, get_elapsed_time_in_ms(tmp_timer));
      start_timer(tmp_timer);
#endif

      // ---------------------------------------------------------------
      // Cameras update
      // ---------------------------------------------------------------
      camera_system::update(w.camera_system);

#if CHRONO_STEPS
      stop_timer(tmp_timer);
      app_watcher::save_value(WL_CAMERA_SYSTEM_UPDATE, get_elapsed_time_in_ms(tmp_timer));
      start_timer(tmp_timer);
#endif

      // ---------------------------------------------------------------
      // Scene graph to sub systems
      // ---------------------------------------------------------------

      for (e = idlut::begin(w.units); e < end; e++) {
        u  = &e->value;

        // scene to movers
        cr = u->components + u->mover_offset;
        for (u32 i = 0; i < u->num_movers; i++) {
          if (scene_system::changed(w.scene_system, u->scene_graph, cr[i].node) && u->mover != i) {
            const MoverResource &mr = unit_resource::movers(u->resource)[i];
            physics_system::teleport_mover(w.physics_world, cr[i].component,
                                           scene_system::world_translation(w.scene_system, u->scene_graph, cr[i].node)
                                           + glm::vec3(mr.offset[0], mr.offset[1], 0));
          }
          u->mover = UINT32_MAX;
        }

        // scene to actors
        cr = u->components + u->actor_offset;
        for (u32 i = 0; i < u->num_actors; i++) {
          if (w.physics_world.simulate
              && !physics_system::is_static(w.physics_world, cr[i].component)
              && !physics_system::is_kinematic(w.physics_world, cr[i].component))
              continue; // skip dynamic actors, they update their position on their own
          if (!scene_system::changed(w.scene_system, u->scene_graph, cr[i].node))
            continue; // skip the sync if the node is not dirty
          pose::transform_world_pose(physics_system::get_pose(w.physics_world, cr[i].component),
                                     scene_system::world_pose(w.scene_system, u->scene_graph, cr[i].node));
        }

        // scene to sprites
        cr = u->components + u->sprite_offset;
        for (u32 i = 0; i < u->num_sprites; i++) {
          if (!scene_system::changed(w.scene_system, u->scene_graph, cr[i].node))
            continue; // skip the sync if the node is not dirty
          Pose &pose = sprite_system::get_pose(w.sprite_system, cr[i].component);
          glm::mat4 m = scene_system::world_pose(w.scene_system, u->scene_graph, cr[i].node);
          pose::transform_world_pose(pose, m);
        }

        // scene to texts
        cr = u->components + u->text_offset;
        for (u32 i = 0; i < u->num_texts; i++) {
          if (!scene_system::changed(w.scene_system, u->scene_graph, cr[i].node))
            continue; // skip the sync if the node is not dirty
          pose::transform_world_pose(text_system::get_pose(w.text_system, cr[i].component),
                                     scene_system::world_pose(w.scene_system, u->scene_graph, cr[i].node));
        }
      }

      // particle update
      particle_system::update(w.particle_system, delta_time);

#if CHRONO_STEPS
      stop_timer(tmp_timer);
      app_watcher::save_value(WL_SCENE_TO_PHYSICS, get_elapsed_time_in_ms(tmp_timer));
      start_timer(tmp_timer);
#endif

      // synchronize AABB
      //sprite_system::update_aabb(w.sprite_system);

#if CHRONO_STEPS
      stop_timer(tmp_timer);
      app_watcher::save_value(WL_CULLING_SYSTEM_UPDATE, get_elapsed_time_in_ms(tmp_timer));
      start_timer(tmp_timer);
#endif

      // ---------------------------------------------------------------
      // Update finilization
      // ---------------------------------------------------------------
      scene_system::finalize_update(w.scene_system);
      physics_system::finalize_update(w.physics_world);

#if CHRONO_STEPS
      stop_timer(tmp_timer);
      app_watcher::save_value(WL_SCENE_SYSTEM_FINALIZE, get_elapsed_time_in_ms(tmp_timer));
      start_timer(tmp_timer);
#endif

      // ---------------------------------------------------------------
      // Sub systems update
      // ---------------------------------------------------------------
      text_system::update(w.text_system);
      sprite_system::update(w.sprite_system);
      geometric_system::update(w.geometric_system);

#if CHRONO_STEPS
      stop_timer(tmp_timer);
      stop_timer(frame_timer);
      app_watcher::save_value(WL_SUB_SYSTEMS_UPDATE, get_elapsed_time_in_ms(tmp_timer));
      app_watcher::save_value(WL_SCENE_UPDATE, get_elapsed_time_in_ms(frame_timer));
#endif
    }

    void update(u64 world, f64 delta_time)
    {
      update_scene(world, delta_time);
      update_anims(world, delta_time);
    }

    // UNIT

    u64 spawn_unit(u64 world, const char *name, const glm::vec3 &translation, const glm::quat &rotation, const glm::vec3 &scale)
    {
      return _spawn_unit(world, murmur_hash_32(name), translation, rotation, scale);
    }

    void despawn_unit(u64 world, u64 unit)
    {
      _despawn_unit(application::world(world), unit);
    }

    u32 num_units(u64 world)
    {
      return idlut::size(application::world(world).units);
    }

    // Returns all the units of the 'world'.
    void get_units(u64 world, Array<u64> &units)
    {
      World &w = application::world(world);

      array::resize(units, 0);
      array::reserve(units, idlut::size(w.units));

      IdLookupTable<Unit>::Entry *e, *end = idlut::end(w.units);
      for (e = idlut::begin(w.units); e < end; e++)
        array::push_back(units, e->id);
    }

    void link_unit(u64 world, u64 child, u64 parent, i32 node)
    {
      World &w = application::world(world);
      const Unit &c = *idlut::lookup(w.units, child);
      const Unit &p = *idlut::lookup(w.units, parent);

      scene_system::link(w.scene_system, c.scene_graph, p.scene_graph, node);
    }

    void unlink_unit(u64 world, u64 child)
    {
      World &w = application::world(world);
      const Unit &c = *idlut::lookup(w.units, child);

      scene_system::unlink(w.scene_system, c.scene_graph);
    }

    // SPRITE

    u64 spawn_sprite(u64 world, const char *name, const glm::vec3 &translation, const glm::quat &rotation, const glm::vec3 &scale)
    {
      return _spawn_sprite(world, murmur_hash_32(name), translation, rotation, scale);
    }
    
    void despawn_sprite(u64 world, u64 sprite)
    {
      World &w = application::world(world);
      sprite_system::destroy(w.sprite_system, sprite);
    }

    // TEXT

    u64 spawn_text(u64 world, const char *font, const char *text, TextAlign align, const glm::vec3 &position, const glm::quat &rotation)
    {
      World &w  = application::world(world);
      u64   id  = text_system::create(w.text_system, (FontResource*)resource_manager::get(RESOURCE_TYPE_FONT, murmur_hash_32(font)), text, align);
      Pose  &p  = text_system::get_pose(w.text_system, id);

      pose::set_local_translation(p, position);
      pose::set_local_rotation(p, rotation);
      return id;
    }

    void despawn_text(u64 world, u64 text)
    {
      text_system::destroy(application::world(world).text_system, text);
    }

    // CAMERA

    u64 spawn_camera(u64 world, f32 aspect, const glm::vec3 &position, const glm::quat &rotation)
    {
      Camera camera;
      camera.aspect = aspect;
      camera.fov    = glm::radians(45.0f);

      pose::set_local_translation(camera.pose, position);
      pose::set_local_rotation(camera.pose, rotation);

      return idlut::add((*idlut::lookup(app->worlds, world))->camera_system, camera);
    }

    void despawn_camera(u64 world, u64 camera)
    {
      idlut::remove((*idlut::lookup(app->worlds, world))->camera_system, camera);
    }

    // GEOMETRY

    u64 spawn_line(u64 world, const glm::vec3 &a, const glm::vec3 &b, const Color &color, const glm::vec3 &position, const glm::quat &rotation)
    {
      World &w  = application::world(world);
      u64    id = geometric_system::create_line(w.geometric_system, a, b, color);
      Pose  &p  = geometric_system::get_pose(w.geometric_system, id);

      pose::set_local_translation(p, position);
      pose::set_local_rotation(p, rotation);

      return id;
    }

    u64 spawn_chain(u64 world, const glm::vec3 *vertices, u32 num_vertices, Color &color, const glm::vec3 &position, const glm::quat &rotation)
    {
      World &w  = application::world(world);
      u64    id = geometric_system::create_chain(w.geometric_system, vertices, num_vertices, color);
      Pose  &p  = geometric_system::get_pose(w.geometric_system, id);

      pose::set_local_translation(p, position);
      pose::set_local_rotation(p, rotation);

      return id;
    }

    u64 spawn_polygon(u64 world, const glm::vec3 *vertices, u32 num_vertices, Color &color, const glm::vec3 &position, const glm::quat &rotation)
    {
      World &w  = application::world(world);
      u64    id = geometric_system::create_polygon(w.geometric_system, vertices, num_vertices, color);
      Pose  &p  = geometric_system::get_pose(w.geometric_system, id);

      pose::set_local_translation(p, position);
      pose::set_local_rotation(p, rotation);

      return id;
    }

    u64 spawn_box(u64 world, f32 width, f32 height, const Color &top_color, const Color &bot_color, const glm::vec3 &position, const glm::quat &rotation, bool surface)
    {
      Color colors[4];
      colors[0] = colors[3] = top_color;
      colors[1] = colors[2] = bot_color;
      World &w  = application::world(world);
      u64    id = geometric_system::create_box(w.geometric_system, width, height, colors, surface);
      Pose  &p  = geometric_system::get_pose(w.geometric_system, id);

      pose::set_local_translation(p, position);
      pose::set_local_rotation(p, rotation);

      return id;
    }

    u64 spawn_circle(u64 world, const glm::vec3 &center, f32 radius, const Color &color, const glm::vec3 &position, const glm::quat &rotation, bool surface)
    {
      World &w  = application::world(world);
      u64    id = geometric_system::create_circle(w.geometric_system, center, radius, color, surface);
      Pose  &p  = geometric_system::get_pose(w.geometric_system, id);

      pose::set_local_translation(p, position);
      pose::set_local_rotation(p, rotation);

      return id;
    }

    void despawn_geometry(u64 world, u64 geometry)
    {
      GeometricSystem &s = (*idlut::lookup(app->worlds, world))->geometric_system;
      geometric_system::destroy(s, geometry);
    }
    
    // LEVEL
    
    u64 load_level(u64 world, const char *name, const glm::vec3 &position, const glm::quat &rotation)
    {
      const LevelResource *res = (LevelResource*)resource_manager::get(RESOURCE_TYPE_LEVEL, murmur_hash_32(name));

      u32 num_units = level_resource::num_units(res);
      const LevelResource::Resource *res_units = level_resource::get_units(res);

      u32 num_sprites = level_resource::num_spritess(res);
      const LevelResource::Resource *res_sprites = level_resource::get_sprites(res);

      Level *level = MAKE_NEW((*app->_a), Level, (*app->_a));

      glm::mat4 mlevel;
      {
        DecomposedMatrix dm;
        dm.translation = position;
        dm.rotation    = rotation;
        dm.scale       = glm::vec3(1, 1, 1);
        compose_mat4(mlevel, dm);
      }

      load_level_resource(level->units, num_units, world, res_units, mlevel, _spawn_unit);
      load_level_resource(level->sprites, num_sprites, world, res_sprites, mlevel, _spawn_sprite);
      
      return idlut::add((*idlut::lookup(app->worlds, world))->levels, level);
    }

    void unload_level(u64 world, u64 level)
    {
      _unload_level(application::world(world), level);
    }

    void physics_simulations(u64 world, bool value)
    {
      physics_system::allow_simulations(application::world(world).physics_world, value);
    }


    // ACTOR

    u64 spawn_actor_chain(u64 world, const glm::vec2 *vertices, u32 num_vertices,
                          bool kinematic, bool dynamic, bool gravity, bool trigger,
                          const char *collision_filter, const char *material,
                          const glm::vec3 &position, const glm::quat &rotation)
    {
      World &w  = application::world(world);
      u64    id = _create_actor(w.physics_world, SHAPE_TYPE_CHAIN,
                                (f32*)vertices, num_vertices * 2,
                                kinematic, dynamic, gravity, trigger,
                                collision_filter, material);

      Pose  &p  = physics_system::get_pose(w.physics_world, id);

      pose::set_local_translation(p, position);
      pose::set_local_rotation(p, rotation);

      return id;
    }

    u64 spawn_actor_polygon(u64 world, const glm::vec2 *vertices, u32 num_vertices,
                            bool kinematic, bool dynamic, bool gravity, bool trigger,
                            const char *collision_filter, const char *material,
                            const glm::vec3 &position, const glm::quat &rotation)
    {
      World &w  = application::world(world);
      u64    id = _create_actor(w.physics_world, SHAPE_TYPE_POLYGON,
                                (f32*)vertices, num_vertices * 2,
                                kinematic, dynamic, gravity, trigger,
                                collision_filter, material);

      Pose  &p  = physics_system::get_pose(w.physics_world, id);

      pose::set_local_translation(p, position);
      pose::set_local_rotation(p, rotation);

      return id;
    }

    u64 spawn_actor_circle(u64 world, f32 radius,
                           bool kinematic, bool dynamic, bool gravity, bool trigger,
                           const char *collision_filter, const char *material,
                           const glm::vec3 &position, const glm::quat &rotation)
    {
      World &w  = application::world(world);
      u64    id = _create_actor(w.physics_world, SHAPE_TYPE_CIRCLE,
                                &radius, 1,
                                kinematic, dynamic, gravity, trigger,
                                collision_filter, material);

      Pose  &p  = physics_system::get_pose(w.physics_world, id);

      pose::set_local_translation(p, position);
      pose::set_local_rotation(p, rotation);

      return id;
    }

    u64 spawn_actor_box(u64 world, f32 width, f32 height,
                        bool kinematic, bool dynamic, bool gravity, bool trigger,
                        const char *collision_filter, const char *material,
                        const glm::vec3 &position, const glm::quat &rotation)
    {
      f32 box[2] ={ width, height };

      World &w  = application::world(world);
      u64    id = _create_actor(w.physics_world, SHAPE_TYPE_BOX,
                                box, 2,
                                kinematic, dynamic, gravity, trigger,
                                collision_filter, material);

      Pose  &p  = physics_system::get_pose(w.physics_world, id);

      pose::set_local_translation(p, position);
      pose::set_local_rotation(p, rotation);

      return id;
    }

    void despawn_actor(u64 world, u64 actor)
    {
      physics_system::destroy_actor(application::world(world).physics_world, actor);
    }

    u64 spawn_particles(u64 world, const char *effect, const glm::vec3 &position, const glm::quat &rotation)
    {
      World &w  = application::world(world);
      u64    id = particle_system::create(w.particle_system, (ParticleResource*)resource_manager::get(RESOURCE_TYPE_PARTICLE, murmur_hash_32(effect)));
      Pose  &p  = particle_system::get_pose(w.particle_system, id);
      pose::set_local_translation(p, position);
      pose::set_local_rotation   (p, rotation);
      particle_system::start(w.particle_system, id);
      return id;
    }

    void despawn_particles(u64 world, u64 effect)
    {
      World &w = application::world(world);
      particle_system::destroy(w.particle_system, effect);
    }

    void stop_spawning_particles(u64 world, u64 effect)
    {
      World &w = application::world(world);
      particle_system::stop(w.particle_system, effect);
    }

    void move_particles(u64 world, u64 effect, const glm::vec3 &position, const glm::quat &rotation)
    {
      World &w = application::world(world);
      Pose  &p =  particle_system::get_pose(w.particle_system, effect);
      pose::set_local_translation(p, position);
      pose::set_local_rotation(p, rotation);
    }

    u32 num_particles(u64 world, u64 effect) {
      World &w = application::world(world);
      return idlut::lookup(w.particle_system.emitters, effect)->num_particles;
    }
  }

  World::World(Allocator &a) :
    scene_system(a),
    culling_system(a),
    physics_world(.016f, a),
    units(a),
    text_system(a),
    actor_unit(a),
    camera_unit(a),
    text_unit(a),
    sprite_unit(a),
    sprite_system(a, culling_system),
    total_time(0.f),
    delta_time(0.f),
    animation_player(a),
    camera_system(a),
    levels(a),
    geometric_system(a),
    particle_system(a),
    mover_unit(a){}

  World::~World()
  {
    // Cleans levels
    while (idlut::size(levels))
      _unload_level(*this, idlut::begin(levels)->id);

    // Cleans units
    IdLookupTable<Unit>::Entry *e, *end = idlut::end(units);
    for (e = idlut::begin(units); e < end; e++)
      _despawn_unit(*this, e->id);
  }

  Level::Level(Allocator &a) : units(a), sprites(a) {}

}