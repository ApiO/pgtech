#pragma once

#include <runtime/idlut.h>
#include <renderer/renderer_types.h>
#include <application_types.h>
#include "sprite_types.h"
#include <pose.h>

namespace pge
{
  namespace sprite_system
  {
    u64  create      (SpriteSystem &system, const SpriteResource *resource, u32 group, u32 order);
    void destroy     (SpriteSystem &system, u64 sprite);
    void update      (SpriteSystem &system);
    void update_aabb (SpriteSystem &system);
    void gather      (SpriteSystem &system);
    void draw        (const Graphic *graphics, u32 num_items, RenderBuffer &render_buffer);

    i32  get_frame         (SpriteSystem &system, u64 sprite, const char *name);
    i32  get_setup_frame   (SpriteSystem &system, u64 sprite);
    i32  get_current_frame (SpriteSystem &system, u64 sprite);
    i32  get_num_frames    (SpriteSystem &system, u64 sprite);
    void get_color         (SpriteSystem &system, u64 sprite, Color &color);
    void get_size          (SpriteSystem &system, u64 sprite, glm::vec2 &size);
    
    void set_frame (SpriteSystem &system, u64 sprite, i32 frame);
    void set_color (SpriteSystem &system, u64 sprite, const Color &color);

    Pose &get_pose (SpriteSystem &system, u64 sprite);
  }
  inline SpriteSystem::SpriteSystem(Allocator &a, CullingSystem &cs) : sprites(a), culling_system(&cs) {};
}