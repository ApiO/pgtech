#pragma once

#include <runtime/types.h>
#include <runtime/memory_types.h>
#include "camera.h"

namespace app
{
  struct Screen
  {
    pge::i32 width;
    pge::i32 height;
    pge::f32 w2;
    pge::f32 h2;
  };

  extern Screen global_screen;

  extern pge::u64 global_simple_package;
  extern pge::u64 global_samp_ball_pkg;

  extern pge::u64 global_game_world;
  extern pge::u64 global_gui_world;

  extern pge::u64 global_viewport;

  extern pge::u32 global_pad_index;

  extern Camera global_game_camera;
  extern Camera global_gui_camera;
  
  extern char *global_font_name;
  extern bool  global_debug_physic;

  extern char *global_sample_desciption;
}