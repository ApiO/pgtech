#pragma once

#include <runtime/memory_types.h>
#include <data/package.h>

#include <engine/pge_types.h>

#include "animation/animation_types.h"
#include "scene/scene_types.h"
#include "physics/physics_types.h"
#include "audio/audio_types.h"
#include "text/text_types.h"
#include "sprite/sprite_types.h"
#include "particle/particle_types.h"
#include "geometry/geometric_types.h"
#include "culling/culling_types.h"
#include "camera/camera_types.h"
#include "resource/unit_resource.h"

namespace pge
{
  struct Unit;
  
  struct Viewport
  {
    u32 screen_x;
    u32 screen_y;
    u32 width;
    u32 height;
  };
  
  struct ComponentRef 
  {
    u64 component; // id of the component
    i32 node;      // index of the unit node the component is attached to
  };

  struct Unit
  {
    u64 scene_graph;
    u64 player_session;

    const UnitResource    *resource;
    const AnimsetResource *animation_set;

    // component references
    ComponentRef *components;

    u32 num_actors;
    u32 actor_offset;
    
    u32 num_sprites;
    u32 sprite_offset;

    u32 num_movers;
    u32 mover_offset;

    u32 num_texts;
    u32 text_offset;

    u32 num_cameras;
    u32 camera_offset;

    u32 mover; // the mover used to move this unit (UINT32_MAX = no mover used)
  };
  
  struct UnitRef 
  { // used as hash value with component ids as key (component to unit)
    u64 unit;  // id of the unit the component is attached to
    i32 node;  // index of the unit node the component is attached to
  };

  struct Level
  {
    Level(Allocator &a);
    Array<u64> units;
    Array<u64> sprites;
  };
    
  struct World
  {
    World(Allocator &a);
    ~World();

    f64 total_time;
    f64 delta_time;

    SceneSystem     scene_system;
    TextSystem      text_system;
    GeometricSystem geometric_system;
    AnimationPlayer animation_player;
    AudioWorld      audio_world;
    PhysicsWorld    physics_world;
    CullingSystem   culling_system;
    SpriteSystem    sprite_system;
    CameraSystem    camera_system;
    ParticleSystem  particle_system;

    IdLookupTable<Unit>     units;
    IdLookupTable<Level*>   levels;

    // component to unit reference
    Hash<UnitRef> actor_unit;
    Hash<UnitRef> sprite_unit;
    Hash<UnitRef> mover_unit;
    Hash<UnitRef> text_unit;
    Hash<UnitRef> camera_unit;
  };
}