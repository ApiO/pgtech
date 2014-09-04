#include <windows.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <stdio.h>
#include <time.h>
#include <imgui/imgui.h>
#include <imgui/imguiRenderGL3.h>
#include <glm/gtx/vector_angle.hpp>

#include <engine/pge.h>
#include <runtime/memory.h>
#include <runtime/json.h>
#include <runtime/array.h>
#include <runtime/assert.h>
#include <runtime/trace.h>

#include "application_types.h"
#include "forms/forms.h"
#include "handlers/resource_handler.h"
#include "handlers/selection_handler.h"
#include "handlers/project_handler.h"
#include "handlers/level_handler.h"
#include "handlers/action_handler.h"
#include "controls/input_control.h"
#include "utils/camera.h"
#include "utils/camera_control.h"
#include "utils/fps.h"
#include "crash_handler.h"


using namespace pge;
using namespace app::bases;
using namespace app::controls;
using namespace app::forms;
using namespace app::handlers;
using namespace app::utils;

const char *LOG_FILE_NAME = "log.txt";

namespace app
{
  struct WindowConfig
  {
    i32 x, y;
    i32 width, height;
    bool resizable;
    bool full_screen;
  };

  struct UserInterface
  {
    struct Orientation
    {
      bool      update;
      glm::vec2 state_position;
      glm::vec3 euler;
    };
    bool show_interface;
    bool show_aabb;
    bool show_physics;
    bool mouse_right_drag;
    bool quit;
    Orientation orientation;
    //u64    bg_world;
    //Camera bg_camera;
    Camera camera;
    glm::vec2 drag_screen_pose;
  };

  UserInterface   ui;
  WindowConfig    game_window_config;
  WindowConfig    editor_window_config;
  ApplicationData app_data;
  Screen          screen;

  const char *SETTINGS_FILE_NAME = "settings.pgconf";
  const char *FONT_NAME = "fonts/consolas.24/consolas.24";
  //const Color AXES_COLOR(92.f, 92.f, 92.f, 255.f);
  const f32   MOUSE_SPEED = 8.f;

  char app_path[MAX_PATH];
  char source[MAX_PATH];
  char data[MAX_PATH];

  inline void set_screen(i32 width, i32 height)
  {
    screen.height = height;
    screen.width = width;
    screen.max_window_height = screen.height - (2 * IMGUI_ITEM_HEIGHT);
  }
}


// internal utils

namespace app
{
  static void set_form_position()
  {
    property->set_position(screen.width - WINDOW_MAX_WIDTH - WINDOW_MARGIN, WINDOW_MARGIN);
    layers->set_position(screen.width - WINDOW_MAX_WIDTH - WINDOW_MARGIN, screen.height - WINDOW_MARGIN);
    menu->set_position(WINDOW_MARGIN, screen.height - WINDOW_MARGIN);
  }

  static void on_window_resize(i32 width, i32 height)
  {
    set_screen(width, height);
    application::set_viewport(app_data.viewport, 0, 0, width, height);
    set_form_position();
  }
  
  inline void handle_navigation(f64 dt)
  {
    // handles camera orientation

    if (!has_input_focused() && !ui.mouse_right_drag && keyboard::pressed(KEYBOARD_KEY_LEFT_ALT)){
      ui.orientation.update = true;
      ui.orientation.state_position = glm::vec2(screen.width, screen.height)  * .5f;
      ui.orientation.euler = ui.camera.get_euler_angles();
      window::display_cursor(false);
      mouse::set_position(ui.orientation.state_position);
    }

    if (ui.orientation.update && keyboard::released(KEYBOARD_KEY_LEFT_ALT)){
      ui.orientation.update = false;
      window::display_cursor(true);
    }

    if (ui.orientation.update && keyboard::button(KEYBOARD_KEY_LEFT_ALT)){
      glm::vec2 mp = mouse::get_position();

      if (ui.orientation.state_position != mp)
      {
        f32 w2 = screen.width * .5f;
        f32 h2 = screen.height * .5f;

        ui.orientation.state_position = glm::vec2(w2, h2);
        mouse::set_position(ui.orientation.state_position);
        ui.orientation.state_position = mp;

        ui.orientation.euler.y += (f32)((w2 - mp.x) * MOUSE_SPEED * dt);
        ui.orientation.euler.x += (f32)((h2 - mp.y) * MOUSE_SPEED * dt);

        ui.camera.set_euler_angles(ui.orientation.euler);
      }
    }

    // handles keyboard moves
    if (has_input_focused()) return;

    if (keyboard::button(KEYBOARD_KEY_Q)){
      ui.camera.move(CAMERA_MOVE_LEFT, dt);
    }
    else if (keyboard::button(KEYBOARD_KEY_D)){
      ui.camera.move(CAMERA_MOVE_RIGHT, dt);
    }

    if (keyboard::button(KEYBOARD_KEY_Z)){
      ui.camera.move(CAMERA_MOVE_FORWARD, dt);
    }
    else if (keyboard::button(KEYBOARD_KEY_S) && !keyboard::button(KEYBOARD_KEY_LEFT_CONTROL)){
      ui.camera.move(CAMERA_MOVE_BACKWARD, dt);
    }

  }
}

// engine API usage

namespace app
{
  void init(void)
  {
    // WINDOW INITS

    editor_window_config.full_screen = false;
    editor_window_config.resizable = true;
    {
      i32 width, height;
      window::get_screen_resolution(width, height);
      editor_window_config.width = (i32)(width * 4.f / 5.f);
      editor_window_config.height = (i32)(height * 4.f / 5.f);
    }

    window::create("pgtech - Level editor v1.0",
      editor_window_config.width,
      editor_window_config.height,
      editor_window_config.full_screen,
      editor_window_config.resizable);

    window::set_on_resized(on_window_resize);
    set_screen(editor_window_config.width, editor_window_config.height);
    set_form_position();

    // ENGINE INITS

    application::set_autoload(true);

    window::get_resolution(screen.width, screen.height);

    // init ui render stuff
    app_data.world = application::create_world();
    ui.camera.init(app_data.world, CAMERA_MODE_FREE, (f32)screen.width, (f32)screen.height);
    ui.camera.set_near_range(0.1f);
    ui.camera.set_far_range(10000.f);
    {
      glm::vec3 cam_pos(0.f, 0.f, 664.f);
      ui.camera.set_position(cam_pos);
      camera_control::setup(ui.camera, cam_pos);
    }

    app_data.camera = ui.camera.get_id();
    app_data.viewport = application::create_viewport(0, 0, screen.width, screen.height);

    world::physics_simulations(app_data.world, false);
    application::show_culling_debug(ui.show_aabb);
    physics::show_debug(ui.show_physics);


    // spawns world origin
    const f32 LINE_LEN = 200.f;
    world::spawn_line(app_data.world, glm::vec3(0.f), glm::vec3(0.f, LINE_LEN, 0.f), GREEN_COLOR, IDENTITY_TRANSLATION, IDENTITY_ROTATION);
    world::spawn_line(app_data.world, glm::vec3(0.f), glm::vec3(LINE_LEN, 0.f, 0.f), RED_COLOR,   IDENTITY_TRANSLATION, IDENTITY_ROTATION);
    world::spawn_line(app_data.world, glm::vec3(0.f), glm::vec3(0.f, 0.f, LINE_LEN), BLUE_COLOR,  IDENTITY_TRANSLATION, IDENTITY_ROTATION);

    /*
    // init bg render stuff
    ui.bg_world = application::create_world();
    ui.bg_camera.init(ui.bg_world, CAMERA_MODE_OTRHO, (f32)screen.width, (f32)screen.height);
    ui.bg_camera.set_orthographic_projection(f32(-screen.width*.5f), f32(screen.width*.5f), f32(-screen.height*.5f), f32(screen.height*.5f));
    ui.bg_camera.set_near_range(-1.0f);
    ui.bg_camera.set_far_range(1.0f);

    {
      // draws axes
      const i32 num_lines = 3;
      f32 x_offset = (f32)(screen.width / (num_lines + 1));
      f32 y_offset = (f32)(screen.height / (num_lines + 1));

      f32 w2 = screen.width  * .5f;
      f32 h2 = screen.height * .5f;

      f32 x, y;
      for (i32 i = 1; i <= num_lines; i++)
      {
        x = -w2 + x_offset * i;
        y = -h2 + y_offset * i;
        world::spawn_line(ui.bg_world, glm::vec3(0.f, -h2, 0.f), glm::vec3(0.f, h2, 0.f), AXES_COLOR, glm::vec3(x, 0.f, 0.f), IDENTITY_ROTATION);
        world::spawn_line(ui.bg_world, glm::vec3(-w2, 0.f, 0.f), glm::vec3(w2, 0.f, 0.f), AXES_COLOR, glm::vec3(0.f, y, 0.f), IDENTITY_ROTATION);
      }
    }
    //*/

    // imgui render data
    app_data.screen_projection = glm::ortho(-screen.width*.5f, screen.width*.5f, -screen.height*.5f, screen.height*.5f, -1.f, 1.f);

    fps::init();
  }

  void update(f64 delta_time)
  {
    fps::update(delta_time);

    // catch mouse updates
    if (mouse::pressed(MOUSE_KEY_1))
      focusable::reset();

    if (mouse::button(MOUSE_KEY_3)){
      ui.camera.set_position(glm::vec3(0, 0, DEFAULT_CAMERA_Z));
      ui.camera.set_euler_angles(glm::vec3(0.f));
    }

    app_data.mouse_scroll = -mouse::wheel_scroll();
    app_data.mouse_position = mouse::get_position();
    app_data.mouse_position.y = screen.height - app_data.mouse_position.y;

    // handles keyboard short keys
    if (keyboard::any_pressed())
    {
      if (keyboard::pressed(KEYBOARD_KEY_ESCAPE)){
        selection->clear();
        focusable::reset();
        property->unload();
      }

      // ALT+ F4
      if (keyboard::button(KEYBOARD_KEY_LEFT_ALT) && keyboard::button(KEYBOARD_KEY_F4)) {
        application::quit();
        return;
      }

      // Toggle menu display
      if (!has_input_focused() && keyboard::pressed(KEYBOARD_KEY_I))
        ui.show_interface = !ui.show_interface;

      // Toggle aabb display
      if (!has_input_focused() && keyboard::pressed(KEYBOARD_KEY_B)){
        ui.show_aabb = !ui.show_aabb;
        application::show_culling_debug(ui.show_aabb);
      }
      // Toggle physics display
      if (!has_input_focused() && keyboard::pressed(KEYBOARD_KEY_P)){
        ui.show_physics = !ui.show_physics;
        physics::show_debug(ui.show_physics);
      }
    }

    // Handles 3D navigation
    handle_navigation(delta_time);

    // Handles mouse scroll
    if (!mouse_over_ui())
    {
      if (!keyboard::button(KEYBOARD_KEY_LEFT_SHIFT) && app_data.mouse_scroll)
      {
        glm::vec3 cam_pos = ui.camera.get_position();
        CameraDirection direction;

        if (app_data.mouse_scroll > 0){
          if (cam_pos.z >= CAMERA_Z_MAX) return;
          direction = CameraDirection::CAMERA_MOVE_BACKWARD;
        }
        else{
          if (cam_pos.z <= CAMERA_Z_MIN) return;
          direction = CameraDirection::CAMERA_MOVE_FORWARD;
        }
        ui.camera.move(direction, FRAME_DURATION * abs(app_data.mouse_scroll) * SCROLL_SPEED);
      }
    }

    // Handles move screen
    if (!keyboard::button(KEYBOARD_KEY_LEFT_SHIFT) && !keyboard::button(KEYBOARD_KEY_LEFT_ALT) && mouse::pressed(MOUSE_KEY_2)){
      ui.drag_screen_pose = mouse::get_position();
      ui.drag_screen_pose.y *= -1;
      ui.mouse_right_drag = true;
      return;
    }
    else if (!keyboard::button(KEYBOARD_KEY_LEFT_SHIFT) && mouse::released(MOUSE_KEY_2)){
      ui.mouse_right_drag = false;
    }

    if (ui.mouse_right_drag){
      glm::vec2 mouse_position(mouse::get_position());
      mouse_position.y *= -1;

      glm::vec2 delta(ui.drag_screen_pose - mouse_position);

      if (delta != glm::vec2(0.f)){
        glm::vec3 cam_pos = ui.camera.get_position();
        f32 zoom_acceleration = cam_pos.z / DEFAULT_CAMERA_Z;
        delta.x *= zoom_acceleration;
        delta.y *= zoom_acceleration;

        glm::vec3 pos = cam_pos + glm::vec3(delta, 0.f);
        ui.camera.set_position(pos);
        ui.drag_screen_pose = mouse_position;
      }
    }

    // Handles selection
    if (!mouse_over_ui()
      && mouse::pressed(MOUSE_KEY_1)
      && !menu->is_spawning()
      && !keyboard::button(KEYBOARD_KEY_LEFT_ALT)
      && !keyboard::button(KEYBOARD_KEY_LEFT_SHIFT)){

      selection->select_item();
    }

    selection->update(delta_time);

    // updates forms & tools
    menu->update(delta_time);
    layers->update(delta_time);
    property->update(delta_time);

    camera_control::update(delta_time);
    ui.camera.update();

    world::update_scene(app_data.world, delta_time);
  }

  void render(void)
  {
    //application::render_world(ui.bg_world, ui.bg_camera.get_id(), app_data.viewport);
    application::render_world(app_data.world, ui.camera.get_id(), app_data.viewport);
    selection->draw();
  }

  void shutdown(void)
  {
    bool save = level->is_loaded() && (level->is_edited() || !level->is_created()) &&
      MessageBox(NULL, "Level not saved.\nWould you like to save before quitting ?", "Warning", MB_YESNO | MB_ICONWARNING) == IDYES;

    if (save) level->save();

    ui.camera.shutdown();
    //ui.bg_camera.shutdown();

    project->close();
    level->close();

    //application::destroy_world(ui.bg_world);
    application::destroy_world(app_data.world);
  }

  void synchro()
  {
    if (ui.quit) application::quit();

    menu->synchronize();
    selection->synchronize();

    fps::swap();
  }



  // RENDER stuff

  void render_init(void)
  {
    // Init UI
    char path[MAX_PATH];

    sprintf(path, "%s/resources/DroidSans.ttf", app_path);

    XASSERT(imguiRenderGLInit(path), "Could not init GUI renderer.\n");

    // Check for errors
    GLenum err = glGetError();
    XASSERT(err == GL_NO_ERROR, "OpenGL Error #%u\n", err);
  }

  void render_begin(void)
  {
  }

  void render_end(void)
  {
    if (!ui.show_interface) return;

    // reset OGL states
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glLoadMatrixf((f32*)&app_data.screen_projection);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glUseProgram(0);

    glClearColor(0.3f, 0.3f, 0.3f, 1.f);
    glViewport(0, 0, screen.width, screen.height);

    u8 mouse_button = 0;
    if (mouse::button(MOUSE_KEY_1))
      mouse_button |= IMGUI_MBUT_LEFT;

    imguiBeginFrame((i32)app_data.mouse_position.x, (i32)app_data.mouse_position.y, mouse_button, app_data.mouse_scroll);
    {
      menu->draw();
      layers->draw();
      property->draw();
      fps::draw();
    } imguiEndFrame();

    imguiRenderGLDraw(screen.width, screen.height);

    // Check for errors
    GLenum err = glGetError();
    XASSERT(err == GL_NO_ERROR, "OpenGL Error #%u\n", err);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
  }

  void render_shutdown()
  {
    imguiRenderGLDestroy();

    // Check for errors
    GLenum err = glGetError();
    XASSERT(err == GL_NO_ERROR, "OpenGL Error #%u\n", err);
  }

}


inline void handle_args(int argc, char * argv[])
{
  using namespace app;

  i32 ct = strstr(argv[0], "level_editor.exe") - argv[0] - 1;
  strncat(app_path, argv[0], ct);

  if (argc == 1){
    /*
    strcpy(source, "..\\sandbox\\src\\resources");
    strcpy(data, "..\\sandbox\\src\\data");
    */

    //*
    strcpy(source, "E:\\[Proto_Gecko]\\Clocky\\resources");
    strcpy(data, "E:\\[Proto_Gecko]\\Clocky\\data");
    //*/

    return;
  }

  strcpy(source, argv[1]);
  strcpy(data, argv[2]);
}

void main_app(int argc, char * argv[])
{
  using namespace app;

  handle_args(argc, argv);

  memory_globals::init();
  {
    Allocator &a = memory_globals::default_allocator();
    
    // saves game window config
    {
      StringPool sp(a);
      Json setting_file(a, sp);
      char tmp[160];
      sprintf(tmp, "%s\\%s", data, SETTINGS_FILE_NAME);

      bool r = json::parse_from_file(setting_file, json::root(setting_file), tmp);
      XASSERT(r, "File not found : \"%s\"", tmp);

      const Json::Node &window = json::get_node(setting_file, json::root(setting_file), "window");
      const Json::Node &resolution = json::get_node(setting_file, window.id, "resolution");

      //json::get_string(setting_file, window.id, "title")
      game_window_config.width = json::get_integer(setting_file, resolution.id, 0);
      game_window_config.height = json::get_integer(setting_file, resolution.id, 1);
      game_window_config.full_screen = strcmp(json::get_string(setting_file, window.id, "mode", "WINDOW"), "WINDOW") != 0;
      game_window_config.resizable = json::get_bool(setting_file, window.id, "resizable");
    }

    // inits internal controls/handlers
    resource_handler::init(a);
    level_handler::init(a);
    project_handler::init(a);
    project->load(source, data);
    selection_handler::init(a);
    action_handler::init(a);

    // inits forms
    property_form::init(a);
    layer_form::init(a);
    menu_form::init(a);

    // inits ui
    ui.mouse_right_drag = false;
    ui.show_interface = true;
    ui.show_aabb = false;
    ui.show_physics = false;
    ui.quit = false; 
    ui.orientation.update = false;


    // engine launch 

    application::init(init, update, render, shutdown, synchro,
      render_init, render_begin, render_end, render_shutdown,
      data, a);

    while (!application::should_quit())
      application::update();

    application::shutdown();

    menu_form::shutdown(a);
    layer_form::shutdown(a);
    property_form::shutdown(a);

    action_handler::shutdown(a);
    selection_handler::shutdown(a);
    project_handler::shutdown(a);
    level_handler::shutdown(a);
    resource_handler::shutdown(a);
  }

  memory_globals::shutdown();
}

int main(int argc, char * argv[])
{
  crash_handler::handle(argc, argv, main_app);

  return 0;
}