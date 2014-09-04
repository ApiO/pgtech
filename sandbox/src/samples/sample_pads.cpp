#include <runtime/memory.h>
#include <string.h>
#include <stdio.h>

#include "sample.h"

namespace app
{
  using namespace pge;

  namespace sample_pads
  {
    u64 *pad_texts = NULL;

    glm::vec3 position;

    char p[4096];
    const u32 max_char = 128;
    char tmp[max_char];
    f32 spacing = 20.f;

    void init()
    {
      Allocator &a = memory_globals::default_allocator();
      pad_texts = (u64*)a.allocate(sizeof(u64)*MAX_NUM_PADS);

      position = glm::vec3(global_screen.width*-.5f, global_screen.h2, 0.f);

      for (u32 i = 0; i < MAX_NUM_PADS; i++)
        pad_texts[i] = world::spawn_text(global_game_world, global_font_name, NULL, TEXT_ALIGN_LEFT, position, IDENTITY_ROTATION);

      update(0.f);
    }

    void update(f64 dt)
    {
      (void)dt;
      if (!pad_texts) return;

      u32 spacing_x = 0;
      for (u32 i = 0; i < MAX_NUM_PADS; i++)
      {
        if (pad::disconnected(i))
        {
          text::set_string(global_game_world, pad_texts[i], "");
          continue;
        }
        if (!pad::active(i)) continue;

        /*
        // ne prend pas en compte le changement des axes
        if (!pad::any_pressed(i) && !pad::any_released(i)) {
        spacing_x += world::get_text_width(pad_texts[i]) + 20;
        continue;
        }
        */

        i32 num_buttons = pad::num_buttons(i);
        i32 num_axes = pad::num_axes(i);

        strcpy(p, "\0");
        sprintf(tmp, "[PAD #%d] name: \"%s\"\n  num_buttons: %d\n     num_axes: %d", i, pad::name(i), num_buttons, num_axes);
        strcat(p, tmp);

        for (i32 j = 0; j < num_buttons; j++)
        {
          sprintf(tmp, "\nPAD_KEY_%d %s: %s", j+1, (j < 9 ? " " : ""), pad::button(i, j) ? "X" : " ");
          strcat(p, tmp);
        }

        for (i32 j = 0; j < num_axes; j++)
        {
          sprintf(tmp, "\naxis[%d] : %.5f", j, pad::axes(i, j));
          strcat(p, tmp);
        }

        text::set_local_position(global_game_world, pad_texts[i], glm::vec3(position.x + spacing_x, position.y, .0f));
        text::set_string(global_game_world, pad_texts[i], p);

        glm::vec2 tmp;
        text::get_size(global_game_world, pad_texts[i], tmp);
        spacing_x += (u32)(tmp.x + spacing);
      }

    }

    void shutdown()
    {
      for (u32 i = 0; i < MAX_NUM_PADS; i++)
        world::despawn_text(global_game_world, pad_texts[i]);

      Allocator &a = memory_globals::default_allocator();
      a.deallocate(pad_texts);
    }
  }
}