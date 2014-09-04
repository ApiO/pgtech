#include "sample.h"
#include <string.h>
#include <stdio.h>

namespace app
{
  using namespace pge;

  namespace sample_mouse
  {
    glm::vec3 position(global_screen.width*-.5f, global_screen.h2, 0.f);

    u64 mouse_axes_text, mouse_wheel_text, mouse_btn_text;

    char p[4096];
    const u32 max_char = 128;
    char tmp[max_char];


    void print_mouse_position()
    {
      glm::vec2 position = mouse::get_position();
      strcpy(p, "Mouse");
      sprintf(tmp, "\nx: %.5f  y: %.5f", position.x, position.y);
      strcat(p, tmp);
      text::set_string(global_gui_world, mouse_axes_text, p);
    }

    void print_mouse_wheel()
    {
      strcpy(p, "\0");
      sprintf(p, "\n\nwheel : %d", mouse::wheel_scroll());
      text::set_string(global_gui_world, mouse_wheel_text, p);
    }

    void print_mouse_buttons()
    {
      strcpy(p, "\n\n");
      for (u32 i = 0; i < MOUSE_NUM_BUTTONS; i++)
      {
        sprintf(tmp, "\nbutton %d %s: %s", i+1, (i < 10 ? " " : ""), mouse::button(i) ? "X" : " ");
        strcat(p, tmp);
      }
      text::set_string(global_gui_world, mouse_btn_text, p);
    }


    void init()
    {
      mouse_btn_text  = world::spawn_text(global_gui_world, global_font_name, NULL, TEXT_ALIGN_LEFT, position, IDENTITY_ROTATION);
      mouse_wheel_text  = world::spawn_text(global_gui_world, global_font_name, NULL, TEXT_ALIGN_LEFT, position, IDENTITY_ROTATION);
      mouse_axes_text = world::spawn_text(global_gui_world, global_font_name, NULL, TEXT_ALIGN_LEFT, position, IDENTITY_ROTATION);

      print_mouse_position();
      print_mouse_buttons();
    }

    void update(f64 dt)
    {
      (void)dt;

      print_mouse_position();

      print_mouse_wheel();

      // ne prend pas en compte le changement des axes
      if (!(mouse::any_pressed() || mouse::any_released())) return;

      print_mouse_buttons();

    }

    void shutdown()
    {
      world::despawn_text(global_gui_world, mouse_btn_text);
      world::despawn_text(global_gui_world, mouse_wheel_text);
      world::despawn_text(global_gui_world, mouse_axes_text);
    }
  }
}