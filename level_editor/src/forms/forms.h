#pragma once

#include <application_types.h>

#include <controls/input_control.h>
#include <controls/list_control.h>

using namespace pge;

// level creation
namespace app
{
  namespace forms
  {
    typedef void(*OnClose) (void *owner);

    class LevelCreationForm : public bases::Form
    {
    public:
      LevelCreationForm();
      void update(f64 delta_time);
      void render(void);
      void set_position(i32 x, i32 y);
      void draw(void);
      void show(void);
      void hide(void);
      void *owner;
      OnClose on_close;
    private:
      controls::InputControl input;
    };
  }

  namespace forms
  {
    inline LevelCreationForm::LevelCreationForm()
    {
      SET_MINT(_visible, 0u);

      input.initialize("Relative path", "\0", 0, 0, 84, 280, controls::INPUT_FORMAT_PATH);

      width = 400;
      height = 80;
    }

    inline void LevelCreationForm::set_position(i32 _x, i32 _y)
    {
      x = _x;
      y = _y;
      input.set_position(x + 18, y - 46);
    }

    inline void LevelCreationForm::show()
    {
      SET_MINT(_visible, 1u);
      input.set_focus(true);
    }

    inline void LevelCreationForm::hide()
    {
      input.clear();
      SET_MINT(_visible, 0u);
      input.set_focus(false);
    }
  }
}

// Layer
namespace app
{
  namespace forms
  {
    class LayerForm : public bases::Form
    {
    public:
      LayerForm();
      void update(f64 delta_time);
      void render(void);
      void set_position(i32 x, i32 y);
      void draw(void);
      void show(void);
      void hide(void);
    private:
      bool add_layer;
      bool load_layer;
      u32  load_layer_index;
      i32  form_scroll;
    };

    namespace layer_form
    {
      void init     (Allocator &a);
      void shutdown (Allocator &a);
    }
  }
}

// menu
namespace app
{
  namespace forms
  {
    enum WindowId
    {
      WINDOW_NONE = -1,
      WINDOW_LEVEL,
      WINDOW_UNITS,
      WINDOW_SPRITES
    };

    struct SpawnHandler
    {
      bool handle;
      bool begin;
      bool cancel;
      const char    *name;
      glm::vec2      position_state;
      EditorResource resource;
    };

    class MenuForm : public bases::Form
    {
    public:
      MenuForm(Allocator &a);
      void update(f64 delta_time);
      void render(void);
      void synchronize(void);
      void draw(void);
      void set_position(i32 x, i32 y);
      bool is_spawning(void);
    private:
      i32 current_level_index;
      i32 delta_scroll;
      i32 menu_scroll;
      i32 levels_scroll;
      i32 units_scroll;
      i32 sprites_scroll;
      char source_path[MAX_PATH];
      char data_path[MAX_PATH];
      WindowId sub_window;
      SpawnHandler spawn_handler;
      controls::ListControl levels;
      controls::ListControl units;
      controls::ListControl sprites;
      forms::LevelCreationForm level_form;

      bool quit;
      bool load_level;
      bool save_level;
      bool close_level;
      bool delete_level;

      bool show_help;

      bool project_collapsed;
      bool level_collapsed;
      bool resource_collapsed;
      bool level_created;
      bool level_edited;

      void draw_levels_window(void);
      void draw_units_window(void);
      void draw_sprites_window(void);
    };

    namespace menu_form
    {
      void init(Allocator &a);
      void shutdown(Allocator &a);
    }
  }
}

// property
namespace app
{
  namespace forms
  {
    struct ComponentContext
    {
      // Data
      EditorResource *current_res;
      controls::InputControl translation[3];
      controls::InputControl rotation;
      controls::InputControl scale[2];
      char layer_name[256];
      bool flip_y;
      i32  layer;
      // actions
      bool display_layer_list;
      i32  layer_list_scroll;
      bool toggle_flip;
    };

    struct LayerContext
    {
      // Data
      controls::InputControl  name;
      controls::InputControl  z;
      EditorResource *current_res;
      bool selected;
      bool visible;
      // actions
      bool update_visibility;
      bool update_z;
      bool remove;
      bool select_layer;
    };

    class PropertyForm : public bases::Form
    {
    public:
      PropertyForm();
      void update(f64 delta_time);
      void render(void);
      void draw(void);
      void set_position(i32 x, i32 y);
      void show(void);
      void hide(void);
      void load(const EditorResource &res);
      void refresh(void);
      void unload(void);
    private:
      i32 form_scroll;

      void draw_component_fields(void);
      void draw_layer_fields(void);
      void draw_layer_list(void);
      EditorResource   current_res;
      ComponentContext component_ctx;
      LayerContext     layer_ctx;
      void load_component(const EditorResource &res);
      void load_layer(const u64 &id);
      void update_component_form(f64 delta_time);
      void update_layer_form(f64 delta_time);
    };

    namespace property_form
    {
      void init(Allocator &a);
      void shutdown(Allocator &a);
    }
  }
}

namespace app
{
  using namespace forms;

  extern MenuForm     *menu;
  extern PropertyForm *property;
  extern LayerForm    *layers;

  inline bool mouse_over_ui()
  {
    return menu->is_over() || layers->is_over() || property->is_over();
  }
}