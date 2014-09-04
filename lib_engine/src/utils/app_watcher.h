#pragma once

#include <runtime/types.h>
#include <runtime/memory_types.h>

namespace pge
{
  enum WatcherLog
  {
    WL_APP_INIT = 0,
    WL_DELTA_TIME,
    WL_UPDATE_THREAD,
    WL_RENDER_THREAD,
    WL_MAIN_LOOP,
    WL_SPRITE_RENDER,
    WL_SPRITE_NUM_ITEMS,
    WL_TEXT_RENDER,
    WL_PARTICLE_RENDER,
    WL_PARTICLE_NUM_ITEMS,
    WL_TEXT_NUM_ITEMS,
    WL_PRIMITIVE_RENDER,
    WL_PRIMITIVE_NUM_ITEMS,
    WL_PHYSICS_UPDATES,
    WL_PHYSICS_TO_SCENE,
    WL_SCENE_SYSTEM_UPDATE,
    WL_SCENE_TO_PHYSICS,
    WL_SCENE_SYSTEM_FINALIZE,
    WL_SCENE_UPDATE,
    WL_RENDER_WORLD,
    WL_ANIMATION_UPDATE,
    WL_ANIMATION_TO_SCENE,
    WL_GLFW_SWAP_BUFFER,
    WL_CAMERA_SYSTEM_UPDATE,
    WL_CULLING_SYSTEM_UPDATE,
    WL_SUB_SYSTEMS_UPDATE,
    WL_GAMEPLAY_INIT_CB,
    WL_GAMEPLAY_UPDATE_CB,
    WL_GAMEPLAY_RENDER_CB,
    WL_GAMEPLAY_RENDER_INIT_CB,
    WL_GAMEPLAY_RENDER_BEGIN_CB,
    WL_GAMEPLAY_RENDER_END_CB,
    WL_GAMEPLAY_RENDER_SHUTDOWN_CB
  };

  namespace internal_app_watcher
  {
    extern Hash<Array<f32>*> *elements;
  }

  namespace app_watcher
  {
    void init(u32 num_values, Allocator &a);
    void shut_down(void);

    inline void save_value(WatcherLog type, f64 value)
    {
      array::push_back(**hash::get(*internal_app_watcher::elements, (u64)type), (f32)value);
    }
  }
}