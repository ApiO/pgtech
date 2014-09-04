#include <stdio.h>
#include <string>

#include <engine/pge.h>
#include <runtime/file_system.h>

#include <application_types.h>
#include <handlers/level_handler.h>
#include <handlers/project_handler.h>
#include "forms.h"

using namespace app::handlers;

namespace app
{
  namespace forms
  {
    using namespace utils;

    void LevelCreationForm::update(f64 delta_time)
    {
      // Handles level creation form
      if (BOOL(_visible)) {

        input.update(delta_time);

        if (keyboard::any_pressed()) {
          const char *_input = input.get_value();

          if (keyboard::pressed(KEYBOARD_KEY_ESCAPE)){
            hide();
            if (on_close) on_close(owner);
            return;
          }

          // validation
          if ((keyboard::pressed(KEYBOARD_KEY_ENTER) || keyboard::pressed(KEYBOARD_KEY_KP_ENTER))
            && strlen(_input))
          {
            char path[MAX_PATH];
            sprintf(path, "%s\\%s.pglevel", project->get_source_path(), _input);

            if (file_system::file_exists(path)){
              char msg[512];
              sprintf(msg, "Cannot create level. A level with the same name already exists.\nResource name: %s\nPath: %s", _input, path);
              MessageBox(NULL, msg, "Information", MB_OK | MB_ICONINFORMATION);
              return;
            }

            level->set_level_name(_input);

            input.clear();
            hide();
            if (on_close) on_close(owner);

          }
        }
      }
    }

    void LevelCreationForm::draw()
    {
      if (!BOOL(_visible)) return;

      i32 scroll = 0;

      bool o = imguiBeginScrollArea("Input level name", x, y - height, width, height, &scroll);
      SET_MINT(_over, o ? 1u : 0u);

      imguiSeparatorLine();

      input.draw();

      imguiEndScrollArea();

    }
  }
}
