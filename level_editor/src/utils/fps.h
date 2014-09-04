#pragma once

#include <stdio.h>
#include <imgui/imgui.h>
#include <imgui/imguiRenderGL3.h>
#include <runtime/types.h>
#include <application_types.h>

namespace app
{
  namespace utils
  {
    namespace fps
    {
      using namespace pge;

      f64 elapsed_time;
      u32 frame_count;
      u32 last_frame_count;
      char *front;
      char *back;
      char v[256];
      char v_alt[256];

      inline void init()
      {
        front = v;
        back = v_alt;

        strcpy(v_alt, "0 fps");

        frame_count = 0;
        last_frame_count = 0;
        elapsed_time = 0.f;
      }

      inline void update(f64 delta_time)
      {
        frame_count++;
        elapsed_time += delta_time;

        sprintf(front, "%d fps", last_frame_count);

        if (elapsed_time < 1.f) return;

        last_frame_count = frame_count;
        frame_count = 0u;
        elapsed_time = 0.f;
      }

      inline void draw()
      {
        imguiDrawText(4, 10, imguiTextAlign::IMGUI_ALIGN_LEFT, back, app::WHITE_TEXT);
      }

      inline void swap()
      {
        char *tmp = front;
        front = back;
        back = tmp;
      }
    }
  }
}