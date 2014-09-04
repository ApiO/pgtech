#include <engine/pge.h>
#include <runtime/temp_allocator.h>
#include <runtime/murmur_hash.h>
#include <runtime/assert.h>
#include <runtime/hash.h>
#include <runtime/json.h>

#include <application_types.h>
#include <controls/list_control.h>
#include <handlers/level_handler.h>
#include <handlers/selection_handler.h>
#include <handlers/resource_handler.h>
#include <handlers/project_handler.h>

#include "forms.h"

using namespace app::forms;
using namespace app::handlers;

// internals

namespace
{
  using namespace app;

  const char *menu_items[] = { "Levels", "Units", "Sprites" };
  const u32 MENU_NUM_ITEMS = 3u;

  static void on_creation_form_close(void *owner)
  {
    ((MenuForm*)owner)->enable();
  }

  inline bool confirm_skip_edition()
  {
    return level->is_loaded() && (level->is_edited() || !level->is_created())
      ? MessageBox(NULL, "Level not saved, current editions will be lost.\nContinue anyway ?", "Warning", MB_OKCANCEL | MB_ICONWARNING) == IDOK
      : true;
  }

  struct HelpInfo
  {
    HelpInfo(char *l, i32 c) : label(l), color(c){}
    char *label;
    i32   color;
  };

  const i32 FUNC_DISABLE = imguiRGBA(255, 255, 255, 100);

  const HelpInfo help_infos[] = {
    HelpInfo("Actions:", WHITE_TEXT),
    HelpInfo("   [ I ] Toggle interface", WHITE_TEXT),
    HelpInfo("   [ B ] Toggle bounding boxes", WHITE_TEXT),
    HelpInfo("   [ P ] Toggle physics", WHITE_TEXT),
    HelpInfo("   [ LEFT CLIC ] select / unselect", WHITE_TEXT),
    HelpInfo("   [ RIGHT CLIC DRAG] Move in plan", WHITE_TEXT),
    HelpInfo("   [ WHEEL SCROLL ] Zoom in / out", WHITE_TEXT),
    HelpInfo("   [ WHEEL CLIC ] Reset camera position & orientation", WHITE_TEXT),
    HelpInfo("   [ SHIFT + LEFT CLIC DRAG ] Move selection", WHITE_TEXT),
    HelpInfo("   [ SHIFT + WHEEL SCROLL ] Scale selection", WHITE_TEXT),
    HelpInfo("   [ SHIFT + RIGHT CLIC DRAG ] Rotate selection", WHITE_TEXT),
    HelpInfo("   [ CTRL + LEFT CLIC ] Multi selection", WHITE_TEXT),
    HelpInfo("   [ CTRL + Z ] Undo", FUNC_DISABLE),
    HelpInfo("   [ CTRL + Y ] Redo", FUNC_DISABLE),
    HelpInfo("   [ ALT + mouse move ] Change camera orientation", WHITE_TEXT),
    HelpInfo("   [ Z, Q, S, D ] Move in space", WHITE_TEXT)
  };
  const i32 num_infos = (i32)(sizeof(help_infos) / sizeof(HelpInfo));
  
  inline void display_help()
  {
    const i32 w = 380, h = 360;

    i32 x = (i32)((screen.width - w)  * .5f);
    i32 y = (i32)((screen.height + h) * .5f - 14);
    i32 line_width = 20;

    imguiDrawRoundedRect(x - 20.f, y - h + 30.f, (f32)w, (f32)h, 4.f, imguiRGBA(4, 4, 4, 255));

     for (i32 i = 0; i < num_infos; i++){
       imguiDrawText(x, y, imguiTextAlign::IMGUI_ALIGN_LEFT, help_infos[i].label, help_infos[i].color);
      y -= line_width;
    }
  }

}

namespace app
{
  using namespace controls;


  // Spwan funcs

  namespace forms
  {
    inline void spawn_begin(SpawnHandler &spawn_handler)
    {
      selection->clear();

      spawn_handler.handle = false;
      spawn_handler.begin = true;

      glm::vec3 position;

      i32 layer_index = level->get_active_layer_index();
      f32 z = (layer_index > -1) ?resource->get_layer(layer_index).z : 0.f;

      if (z){
        glm::vec2 p(screen.width*.5f, screen.height*.5f);
        glm::vec3 near_point, far_point, direction;
        f32 distance;

        camera::screen_to_world(app_data.world, app_data.camera, glm::vec3(p.x, p.y, 0), near_point);
        camera::screen_to_world(app_data.world, app_data.camera, glm::vec3(p.x, p.y, 1), far_point);

        direction = glm::normalize(far_point - near_point);

        glm::vec3 plane_normal(0, 0, -1);
        glm::vec3 plane_position(z);

        bool r = intersect_plane(plane_normal, plane_position, near_point, direction, distance);
        ASSERT(r);

        position = glm::vec3(near_point + direction * distance);
        spawn_handler.position_state = mouse::get_position();
      }
      else{
        glm::vec3 p;
        camera::get_local_translation(app_data.world, app_data.camera, p);
        position = glm::vec3(p.x, p.y, 0);
      }

      glm::vec2 scale(1.f);

      spawn_handler.resource.id = resource->create_resource(spawn_handler.resource.type, spawn_handler.name, position, 0, false, scale);
    }

    inline void spawn_update(SpawnHandler &spawn_handler, mint32 &_over)
    {
      if (BOOL(_over)) return;

      glm::vec2 mouse_position(mouse::get_position());

      if (spawn_handler.position_state == mouse_position) return;

      spawn_handler.position_state = mouse::get_position();

      glm::vec3 near_point, far_point, direction;
      f32 distance;

      camera::screen_to_world(app_data.world, app_data.camera, glm::vec3(mouse_position.x, mouse_position.y, 0), near_point);
      camera::screen_to_world(app_data.world, app_data.camera, glm::vec3(mouse_position.x, mouse_position.y, 1), far_point);

      direction = glm::normalize(far_point - near_point);

      glm::vec3 plane_normal(0, 0, -1);

      i32 layer_index = level->get_active_layer_index();
      f32 z = (layer_index > -1) ? resource->get_layer(layer_index).z : 0.f;
      glm::vec3 plane_position(0, 0, z);

      bool r = intersect_plane(plane_normal, plane_position, near_point, direction, distance);
      ASSERT(r);

      glm::vec3 point(near_point + direction * distance);

      resource->set_position(spawn_handler.resource, point);
    }

    inline void spawn_end(SpawnHandler &spawn_handler)
    {
      selection->clear();
      spawn_handler.begin = false;
    }

    inline void spawn_cancel(SpawnHandler &spawn_handler)
    {
      spawn_handler.begin = false;
      spawn_handler.cancel = false;

      resource->destroy_resource(spawn_handler.resource);
    }

    inline void reset_spawn_handler(SpawnHandler &spawn_handler)
    {
      if (spawn_handler.begin)
        spawn_cancel(spawn_handler);

      spawn_handler.handle = false;
      spawn_handler.begin = false;
      spawn_handler.cancel = false;
      spawn_handler.resource.type = RESOURCE_TYPE_NONE;
    }

    static void load_list(ListControl &list, const Array<char*> &data_source)
    {
      char * const * data;
      char * const * end;

      u32 i = 0;
      end = array::end(data_source);

      list.resize(array::size(data_source));

      for (data = array::begin(data_source); data < end; data++){
        ListItem &item = list[i];
        item.data = *data;
        item.selected = false;
        item.enabled = true;
        i++;
      }
    }
  }


  namespace forms
  {
    MenuForm::MenuForm(Allocator &a) : levels(a), units(a), sprites(a)
    {
      menu_scroll = 0;
      levels_scroll = 0;
      units_scroll = 0;
      sprites_scroll = 0;

      width = WINDOW_MAX_WIDTH;

      sub_window = WINDOW_NONE;
      current_level_index = -1;

      project_collapsed = true;
      level_collapsed = false;
      resource_collapsed = false;

      show_help = false;

      load_level = false;
      save_level = false;
      close_level = false;
      delete_level = false;
      quit = false;

      level_form.owner = this;
      level_form.on_close = on_creation_form_close;

      spawn_handler.begin = false;
      spawn_handler.handle = false;
      spawn_handler.cancel = false;
      spawn_handler.resource.type = RESOURCE_TYPE_NONE;


      const i32 max_label_length = 40;
      const char *tmp = project->get_source_path();
      if (strlen(tmp) <= max_label_length)
        sprintf(source_path, tmp);
      else
      {
        source_path[0] = '\0';
        strncat(source_path, tmp, 3);
        strcat(source_path, "...");
        strcat(source_path, tmp + strlen(tmp) - max_label_length + 6);
      }

      tmp = project->get_data_path();
      if (strlen(tmp) <= max_label_length)
        sprintf(data_path, tmp);
      else
      {
        data_path[0] = '\0';
        strncat(data_path, tmp, 3);
        strcat(data_path, "...");
        strcat(data_path, tmp + strlen(tmp) - max_label_length + 7);
      }


      // loading levels
      load_list(levels, project->levels_strings());

      // loading units
      load_list(units, project->units_strings());

      // level sprites
      load_list(sprites, project->sprites_strings());
    }

    bool MenuForm::is_spawning()
    {
      return spawn_handler.handle || spawn_handler.begin;
    }

    void MenuForm::set_position(i32 _x, i32 _y)
    {
      x = _x;
      y = _y;

      level_form.set_position(x + width + WINDOW_MARGIN, y);
    }

    void MenuForm::update(f64 delta_time)
    {
      // Handles ESCAPE
      if (keyboard::pressed(KEYBOARD_KEY_ESCAPE)) {
        if (spawn_handler.begin){
          spawn_handler.begin = false;
          spawn_cancel(spawn_handler);
        }
      }

      if (keyboard::pressed(KEYBOARD_KEY_F1)) {
        show_help = !show_help;
      }

      // Handles level creation form
      if (level_form.is_visible())
        level_form.update(delta_time);

      // SPWAN

      // Handles spawn cancel
      if (spawn_handler.cancel)
        spawn_cancel(spawn_handler);

      // Handles spawn begin
      if (spawn_handler.handle){
        spawn_begin(spawn_handler);
      }
      // Handles spawn update
      else if (spawn_handler.begin){
        spawn_update(spawn_handler, _over);
      }

      // Handles spawn end
      if (!BOOL(_over) && spawn_handler.begin && mouse::pressed(MOUSE_KEY_1)){
        spawn_end(spawn_handler);
        // cleans action
        if (keyboard::button(KEYBOARD_KEY_LEFT_CONTROL)){
          spawn_begin(spawn_handler);
        }
        else {
          levels.clear_selection();
          units.clear_selection();
          sprites.clear_selection();
        }
      }
      
      // LEVEL

      // Handles level load
      if (load_level){
        reset_spawn_handler(spawn_handler);
        level->load(project->levels_strings()[current_level_index]);

        load_level = false;
        SET_MINT(_enable, 1u);
        return;
      }

      // Handles level save
      if ((save_level || (keyboard::button(KEYBOARD_KEY_LEFT_CONTROL) && keyboard::button(KEYBOARD_KEY_S)))
        && level->is_edited())
      {
        reset_spawn_handler(spawn_handler);
        save_level = false;

        if (level->save())
          project->add_level(level->get_level_name());

        SET_MINT(_enable, 1u);
        return;
      }
    }

    void MenuForm::draw()
    {
      if (keyboard::any_pressed()) {
        if (!spawn_handler.begin && keyboard::pressed(KEYBOARD_KEY_ESCAPE)){
          switch (sub_window)
          {
          case WINDOW_UNITS:
            units.clear_selection();
            break;
          case WINDOW_SPRITES:
            sprites.clear_selection();
            break;
          }
          sub_window = WINDOW_NONE;
        }
      }

      // Display level creation form
      if (level_form.is_visible())
        level_form.draw();
      
      // catch mouse wheel for scrolling bar position
      i32 mscroll = 0;
      if (BOOL(_over)) {
        mscroll = app_data.mouse_scroll;
        delta_scroll += mscroll;
      }
      else {
        mscroll = delta_scroll * (delta_scroll > 0 ? -1 : 1);
      }

      if (level_form.is_visible())
        level_form.draw();

      // Finds menu height
      height = 114 + MENU_NUM_ITEMS * IMGUI_ITEM_HEIGHT;

      if (!project_collapsed)
        height += 190;

      if (!resource_collapsed)
        height += 96;

      if (!level_collapsed)
        height += (level->is_loaded()) ? 120 : 50;

      // HEADER

      bool over = imguiBeginScrollArea("Menu", x, y - height, width, height, &menu_scroll);
      SET_MINT(_over, over ? 1u : 0u);

      imguiSeparatorLine();

      // > PROJECT 

      bool toggle = imguiCollapse("Project", NULL, !project_collapsed, BOOL(_enable));
      if (toggle) project_collapsed = !project_collapsed;
      if (!project_collapsed){
        imguiSeparator();
        
        imguiIndent();
        imguiLabel("Source:");
        imguiIndent();
        imguiLabel(source_path);
        imguiUnindent();
        imguiLabel("Data:");
        imguiIndent();
        imguiLabel(data_path);
        imguiUnindent();

        imguiSeparator();

        if (imguiButton("Open", false)) {}
        if (imguiButton("Reload", false)) {}
        if (imguiButton("Close", false)) {}

        imguiUnindent();
        imguiSeparator();
      }

      // > LEVEL 

      toggle = imguiCollapse("Level", level->get_level_name(), !level_collapsed, BOOL(_enable));
      if (toggle) level_collapsed = !level_collapsed;
      if (!level_collapsed){
        imguiIndent();
        imguiSeparator();

        if (!level->is_loaded()) {
          if (imguiButton("New", BOOL(_enable)))
          {
            level_form.show();
            SET_MINT(_enable, 0u);
            sub_window = WINDOW_NONE;
          }
        }
        else {
          if (imguiButton("Save", BOOL(_enable) && level->is_edited() && !save_level)){
            save_level = true;
            SET_MINT(_enable, 0u);
          }

          if (imguiButton("Reload", BOOL(_enable) && level->is_edited() && level->is_created())){
            if (confirm_skip_edition()){
              load_level = true;
              SET_MINT(_enable, 0u);
            }
          }

          if (imguiButton("Delete", BOOL(_enable) && level->is_created())){
            if (confirm_skip_edition() && MessageBox(NULL, "The current level will be permanently erased.\nContinue anyway ?", "Warning", MB_OKCANCEL | MB_ICONWARNING) == IDOK){
              delete_level = true;
              SET_MINT(_enable, 0u);
            }
          }

          if (imguiButton("Close", BOOL(_enable))){
            if (confirm_skip_edition()){
              close_level = true;
              SET_MINT(_enable, 0u);
            }
          }
        }
        imguiUnindent();
        imguiSeparator();
      }

      // > RESOURCE 

      toggle = imguiCollapse("Resources", NULL, !resource_collapsed, BOOL(_enable));
      if (toggle) resource_collapsed = !resource_collapsed;

      if (!resource_collapsed){
        imguiSeparator();
        imguiIndent();
        for (u32 i = 0; i < MENU_NUM_ITEMS; i++) {
          if (imguiItem(menu_items[i], BOOL(_enable) &&
            ((WINDOW_LEVEL != (WindowId)i && level->is_loaded()) ||
            (WINDOW_LEVEL == (WindowId)i && !level->is_loaded())))) {

            if (spawn_handler.begin)
              spawn_handler.cancel = true;
            units.clear_selection();
            sprites.clear_selection();
            level_form.hide();
            sub_window = sub_window == (WindowId)i ? WINDOW_NONE : (WindowId)i;
          }
        }
        imguiUnindent();
        imguiSeparator();
      }

      imguiSeparatorLine();
      imguiSeparator();

      if (imguiButton(show_help ? "Hide help" : "Show help", true))
        show_help = !show_help;

      if (imguiButton("Quit", true)){
        quit = true;
        application::quit();
      }


      imguiEndScrollArea();

      if (show_help)
        display_help();

      // Display sub window
      if (sub_window != -1){
        switch (sub_window)
        {
        case WINDOW_LEVEL:
          draw_levels_window();
          break;
        case WINDOW_UNITS:
          draw_units_window();
          break;
        case WINDOW_SPRITES:
          draw_sprites_window();
          break;
        }
      }
    }

    void MenuForm::synchronize(void)
    {
      // Handles application quit
      if (quit) return;

      // Handles level close
      if (close_level){
        reset_spawn_handler(spawn_handler);
        levels.clear_selection();
        level->close();

        sub_window = WINDOW_NONE;
        current_level_index = -1;
        SET_MINT(_enable, 1u);
        property->unload();
        close_level = false;
        return;
      }

      // Handles level delete
      if (delete_level){
        reset_spawn_handler(spawn_handler);

        project->remove_level(level->get_level_name());
        level->remove();

        sub_window = WINDOW_NONE;
        current_level_index = -1;
        SET_MINT(_enable, 1u);
        property->unload();
        delete_level = false;
        return;
      }

    }

    void MenuForm::draw_levels_window()
    {
      i32 window_height = 70 + IMGUI_ITEM_HEIGHT * levels.size();

      if (window_height > screen.max_window_height)
        window_height = screen.max_window_height;

      bool over = imguiBeginScrollArea("Levels",
        x + width + WINDOW_MARGIN,
        screen.height - window_height - WINDOW_MARGIN,
        WINDOW_MAX_WIDTH,
        window_height, &levels_scroll);

      if (!BOOL(_over))
        SET_MINT(_over, over ? 1u : 0u);

      imguiSeparatorLine();

      char *name = NULL;

      for (u32 i = 0; i < levels.size(); i++)
      {
        name = (char*)levels[i].data;
        if (imguiItem(name, true)){
          load_level = true;
          sub_window = WINDOW_NONE;
          current_level_index = (i32)i;

          if (spawn_handler.begin && spawn_handler.name != name)
            spawn_handler.cancel = true;
        }
      }

      imguiEndScrollArea();
    }

    void MenuForm::draw_units_window()
    {
      char     *name = NULL;
      ListItem *item = NULL;

      i32 window_height = 70 + IMGUI_ITEM_HEIGHT * units.size();

      if (window_height > screen.max_window_height)
        window_height = screen.max_window_height;

      bool over = imguiBeginScrollArea("Units",
        x + width + WINDOW_MARGIN,
        screen.height - window_height - WINDOW_MARGIN,
        WINDOW_MAX_WIDTH,
        window_height, &units_scroll);

      if (!BOOL(_over))
        SET_MINT(_over, over ? 1u : 0u);

      imguiSeparatorLine();

      for (u32 i = 0; i < units.size(); i++)
      {
        item = &units[i];
        name = (char*)item->data;

        if (imguiItem(name, !item->selected && item->enabled))
        {
          units.select(i);

          if (spawn_handler.begin && spawn_handler.name != name)
            spawn_handler.cancel = true;

          spawn_handler.handle = true;
          spawn_handler.begin = false;
          spawn_handler.resource.type = RESOURCE_TYPE_UNIT;
          spawn_handler.name = name;
        }
      }
      imguiEndScrollArea();
    }

    void MenuForm::draw_sprites_window()
    {
      char     *name = NULL;
      ListItem *item = NULL;

      i32 window_height = 70 + IMGUI_ITEM_HEIGHT * sprites.size();

      if (window_height > screen.max_window_height)
        window_height = screen.max_window_height;

      bool over = imguiBeginScrollArea("Sprites",
        x + width + WINDOW_MARGIN,
        screen.height - window_height - WINDOW_MARGIN,
        WINDOW_MAX_WIDTH,
        window_height, &sprites_scroll);

      if (!BOOL(_over))
        SET_MINT(_over, over ? 1u : 0u);

      imguiSeparatorLine();

      for (u32 i = 0; i < sprites.size(); i++)
      {
        item = &sprites[i];
        name = (char*)item->data;

        if (imguiItem(name, !item->selected && item->enabled))
        {
          sprites.select(i);

          if (spawn_handler.begin && spawn_handler.name != name)
            spawn_handler.cancel = true;

          spawn_handler.handle = true;
          spawn_handler.begin = false;
          spawn_handler.resource.type = RESOURCE_TYPE_SPRITE;
          spawn_handler.name = name;
        }
      }
      imguiEndScrollArea();
    }
  }

}

namespace app
{
  forms::MenuForm *menu;

  namespace forms
  {
    namespace menu_form
    {
      void init(Allocator &a)
      {
        menu = MAKE_NEW(a, MenuForm, a);
      }

      void shutdown(Allocator &a)
      {
        if (!menu) return;
        MAKE_DELETE(a, MenuForm, menu);
        menu = NULL;
      }
    }
  }
}