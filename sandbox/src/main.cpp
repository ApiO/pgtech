#include <stdio.h>
#include <time.h>

#include <runtime/types.h>
#include <runtime/memory.h>
#include <runtime/array.h>
#include <runtime/assert.h>
#include <runtime/file_system.h>
#include <engine/pge.h>

#include "samples/sample.h"
#include "FpsWidget.h"
#include "crash_handler.h"


using namespace pge;

namespace
{
  using namespace app;

  const bool USE_DEPLOY_STATMENT = false; // à switcher sur true lors de la publication de la sandbox
  const bool DISPLAY_FPS         = true;
  const bool DISPLAY_SAMPLE_DESC = false;

  typedef void(*Synchro)   (void);
  struct Sample
  {
    Sample(bool deploy, pge::Init i, pge::Update u, Synchro sy, pge::Render r, pge::Shutdown s)
      : deployable(deploy), init(i), update(u), synchronize(sy), render(r), shutdown(s){}
    bool deployable;
    pge::Init     init;
    pge::Update   update;
    Synchro       synchronize;
    pge::Render   render;
    pge::Shutdown shutdown;
  };

  Sample samples[] = {
    Sample(false, sample_default::init, sample_default::update, sample_default::synchronize, NULL, sample_default::shutdown),
    Sample(false, sample_screen_to_world::init, sample_screen_to_world::update, NULL, NULL, sample_screen_to_world::shutdown),
    Sample(false, sample_actor_touch::init, sample_actor_touch::update, NULL, NULL, sample_actor_touch::shutdown),
    Sample(true, sample_actor_raycast::init, sample_actor_raycast::update, NULL, NULL, sample_actor_raycast::shutdown),
    Sample(true, sample_unit::init, NULL, NULL, NULL, sample_unit::shutdown),
    Sample(true, sample_super_spineboy::init, sample_super_spineboy::update, sample_super_spineboy::synchronize, NULL, sample_super_spineboy::shutdown),
    Sample(true, sample_stress_my_balls::init, sample_stress_my_balls::update, NULL, NULL, sample_stress_my_balls::shutdown),
    Sample(false, sample_physics::init, NULL, NULL, NULL, sample_physics::shutdown),
    Sample(false, sample_mr_patate::init, NULL, NULL, NULL, sample_mr_patate::shutdown),
    Sample(false, sample_pads::init, sample_pads::update, NULL, NULL, sample_pads::shutdown),
    Sample(false, sample_mouse::init, sample_mouse::update, NULL, NULL, sample_mouse::shutdown),
    Sample(false, sample_keyboard::init, sample_keyboard::update, NULL, NULL, sample_keyboard::shutdown),
    Sample(false, NULL, sample_audio::update, NULL, NULL, sample_audio::shutdown),
    Sample(false, sample_font::init, NULL, NULL, NULL, sample_font::shutdown),
    Sample(true, sample_geometry::init, sample_geometry::update, NULL, NULL, sample_geometry::shutdown),
    Sample(false, sample_camera::init, sample_camera::update, NULL, NULL, sample_camera::shutdown),
    Sample(false, sample_shape::init, NULL, NULL, NULL, sample_shape::shutdown),
    Sample(true, sample_mover::init, sample_mover::update, NULL, NULL, sample_mover::shutdown),
    Sample(false, sample_sprite::init, sample_sprite::update, NULL, NULL, sample_sprite::shutdown),
    Sample(true, sample_culling::init, sample_culling::update, NULL, sample_culling::render, sample_culling::shutdown),
    Sample(true, sample_blend::init, sample_blend::update, NULL, NULL, sample_blend::shutdown),
    Sample(false, sample_selection::init, sample_selection::update, sample_selection::render, NULL, sample_selection::shutdown),
    Sample(false, sample_level::init, NULL, NULL, NULL, sample_level::shutdown),
    Sample(false, sample_particles::init, sample_particles::update, NULL, NULL, sample_particles::shutdown),
  };

  enum SAMPLE_INDEX
  {
    SAMPLE_INDEX_DEFAULT = 0,
    SAMPLE_INDEX_SCREEN_TO_WORLD,
    SAMPLE_INDEX_ACTOR_TOUCH,
    SAMPLE_INDEX_ACTOR_RAYCAST,
    SAMPLE_INDEX_UNIT,
    SAMPLE_INDEX_SUPER_SPINEBOY,
    SAMPLE_INDEX_STRESS_MY_BALLS,
    SAMPLE_INDEX_PHYSICS,
    SAMPLE_INDEX_MR_PATATE,
    SAMPLE_INDEX_PADS,
    SAMPLE_INDEX_MOUSE,
    SAMPLE_INDEX_KEYBOARD,
    SAMPLE_INDEX_AUDIO,
    SAMPLE_INDEX_FONT,
    SAMPLE_INDEX_GEOMETRY,
    SAMPLE_INDEX_CAMERA,
    SAMPLE_INDEX_SHAPE,
    SAMPLE_INDEX_MOVER,
    SAMPLE_INDEX_SPRITE,
    SAMPLE_INDEX_CULLING,
    SAMPLE_INDEX_BLEND,
    SAMPLE_INDEX_SELECTION,
    SAMPLE_INDEX_LEVEL,
    SAMPLE_INDEX_PARTICLES,
    SAMPLE_INDEX_NUM_INDEX
  };

  static const char *sample_names[] = {
    "default",
    "screen to world",
    "Actor touch",
    "Actor raycast",
    "Unit & animation",
    "Unit linkage and synchronisation",
    "Physics",
    "Physics simple",
    "Mr patate",
    "Pads",
    "Mouse",
    "Keyboard",
    "Audio",
    "Font",
    "Geometry",
    "Camera",
    "Shape",
    "Mover",
    "Sprite",
    "Culling and frustrum",
    "Blend modes",
    "Selection",
    "Level loading",
    "Particle emitter"
  };

  //u32 sample_index = SAMPLE_INDEX_DEFAULT;
  u32 sample_index = SAMPLE_INDEX_FONT;
}

namespace app
{
  u64     global_simple_package, global_samp_ball_pkg;
  u64     global_game_world;
  u64     global_gui_world;
  u64     global_viewport;
  Camera  global_game_camera;
  Camera  global_gui_camera;
  Screen  global_screen;
  char   *global_font_name = "fonts/consolas.24/consolas.24";
  bool    global_debug_physic = false;
  u32     global_pad_index = 0;
  bool    sample_changed = false;
  bool    show_aabb = false;
  char   *global_sample_desciption = NULL;

  u64     main_description;
  u64     description;

  FpsWidget fps_widget;


  void set_window_title()
  {
#if USE_DEPLOY_STATMENT
    window::set_title(sample_names[sample_index]);
    return;
#else
    char buf[512];
    sprintf(buf, "%s (%d/%d)", sample_names[sample_index], sample_index + 1, SAMPLE_INDEX_NUM_INDEX);
    window::set_title(buf);
#endif
  }


  void init(void)
  {
    srand((unsigned int)time(NULL));

    global_simple_package = application::resource_package("simple");
    resource_package::load(global_simple_package);
    resource_package::flush(global_simple_package);

    global_samp_ball_pkg = application::resource_package("sample_ball");
    resource_package::load(global_samp_ball_pkg);
    resource_package::flush(global_samp_ball_pkg);

    pge::window::get_resolution(global_screen.width, global_screen.height);

    global_screen.w2 = global_screen.width * .5f;
    global_screen.h2 = global_screen.height * .5f;

    // SETUPS GAME WORLD & CAMERA
    global_game_world = application::create_world();
    global_game_camera.init(global_game_world, CAMERA_MODE_OTRHO, (f32)global_screen.width, (f32)global_screen.height);
    global_game_camera.set_orthographic_projection(-global_screen.w2, global_screen.w2, -global_screen.h2, global_screen.h2);
    global_game_camera.set_near_range(-1.f);
    global_game_camera.set_far_range(1.f);

    // SETUPS GUI WORLD & CAMERA
    global_gui_world = application::create_world();
    global_gui_camera.init(global_gui_world, CAMERA_MODE_OTRHO, (f32)global_screen.width, (f32)global_screen.height);
    global_gui_camera.set_orthographic_projection(-global_screen.w2, global_screen.w2, -global_screen.h2, global_screen.h2);
    global_gui_camera.set_near_range(-1.f);
    global_gui_camera.set_far_range(1.f);

    // finds first pad slot
    for (u32 i = 0; i < pge::MAX_NUM_PADS; i++)
      if (pad::active(i) && pad::num_buttons(i) < 15){
      global_pad_index = i;
      break;
      }

    global_viewport = application::create_viewport(0, 0, global_screen.width, global_screen.height);

    if (DISPLAY_FPS)
      fps_widget.init(global_gui_world, global_font_name, global_screen.width, global_screen.height);

    i32 desc_pad = 0;

    if (DISPLAY_SAMPLE_DESC){
      desc_pad = 4;
      char main_desc[512] = "PAGE_UP & PAGE_DOWN to switch sample, ECAHP to exit app\nP: Physics\nB: Bounding box";
      main_description = world::spawn_text(global_gui_world, global_font_name, main_desc, TEXT_ALIGN_LEFT, glm::vec3(-global_screen.w2 + 8, global_screen.h2, 0), glm::quat(1, 0, 0, 0));

    }

    description = world::spawn_text(global_gui_world, global_font_name, NULL, TEXT_ALIGN_LEFT, glm::vec3(-global_screen.w2 + 8, global_screen.h2 - 26 * desc_pad, 0), glm::quat(1, 0, 0, 0));


#if USE_DEPLOY_STATMENT
    for (i32 i = 0; i < SAMPLE_INDEX_NUM_INDEX; i++){
      if (!samples[i].deployable) continue;
      sample_index = i;
      break;
    }
#endif

    if (samples[sample_index].init)
      samples[sample_index].init();

    if (!DISPLAY_SAMPLE_DESC)
      global_sample_desciption = NULL;

    char tmp[1024];
    sprintf(tmp, "[%s]\n%s", sample_names[sample_index], global_sample_desciption ? global_sample_desciption : "");
    text::set_string(global_gui_world, description, tmp);

    physics::show_debug(global_debug_physic);

    set_window_title();
  }

  void handle_sample_navigation()
  {
    bool nav = false;
    i32 sample_pad = 0;

    if (keyboard::pressed(KEYBOARD_KEY_PAGE_UP) && sample_index < (SAMPLE_INDEX_NUM_INDEX - 1))
    {
      //find next
#if USE_DEPLOY_STATMENT
      for (i32 i = sample_index + 1; i < SAMPLE_INDEX_NUM_INDEX; i++){
        if (!samples[i].deployable) continue;
        sample_pad = i - sample_index;
        nav = true;
        break;
      }
# else
      nav = true;
      sample_pad = 1;
#endif
    }
    else if (keyboard::pressed(KEYBOARD_KEY_PAGE_DOWN) && sample_index > 0)
    {
      //find previous
#if USE_DEPLOY_STATMENT
      for (i32 i = sample_index - 1; i > -1; i--)
      {
        if (!samples[i].deployable) continue;
        sample_pad = i - sample_index;
        nav = true;
        break;
      }
# else
      nav = true;
      sample_pad = -1;
#endif
    }

    if (!nav) return;

    if (samples[sample_index].shutdown)
      samples[sample_index].shutdown();

    sample_index += sample_pad;
    global_debug_physic = false;

    global_sample_desciption = NULL;

    if (samples[sample_index].init)
      samples[sample_index].init();

    if (!DISPLAY_SAMPLE_DESC)
      global_sample_desciption = NULL;

    char tmp[1024];
    sprintf(tmp, "[%s]\n%s", sample_names[sample_index], global_sample_desciption ? global_sample_desciption : "");
    text::set_string(global_gui_world, description, tmp);

    physics::show_debug(global_debug_physic);

    int n;
    for (n = 0; n < 10; n++)
      printf("\n\n\n\n\n\n\n\n\n\n");

    sample_changed = true;
  }

  void update(f64 delta_time)
  {
    if (keyboard::any_pressed()){
      if (pad::active(0) && pad::pressed(0, PAD_KEY_7)){
        pge::application::quit();
        return;
      }

      if (keyboard::button(KEYBOARD_KEY_ESCAPE)){
        pge::application::quit();
        return;
      }

      // SAMPLES NAVIGATION
      if (keyboard::pressed(KEYBOARD_KEY_PAGE_UP) || keyboard::pressed(KEYBOARD_KEY_PAGE_DOWN))
        handle_sample_navigation();

      if (keyboard::pressed(KEYBOARD_KEY_B)) {
        show_aabb = !show_aabb;
        application::show_culling_debug(show_aabb);
      }

      if (keyboard::pressed(KEYBOARD_KEY_P)) {
        global_debug_physic = !global_debug_physic;
        physics::show_debug(global_debug_physic);
      }
    }

    if (DISPLAY_FPS)
      fps_widget.update(delta_time);

    global_game_camera.update();
    global_gui_camera.update();

    if (samples[sample_index].update)
      samples[sample_index].update(delta_time);

    world::update(global_game_world, delta_time);
    world::update(global_gui_world, delta_time);

    if (samples[sample_index].synchronize){
      samples[sample_index].synchronize();
      world::update(global_game_world, 0.f);
    }
  }

  void render_end()
  {
    if (!sample_changed) return;

    sample_changed = false;

    set_window_title();
  }

  void render(void)
  {
    application::render_world(global_game_world, global_game_camera.get_id(), global_viewport);

    if (samples[sample_index].render)
      samples[sample_index].render();

    application::render_world(global_gui_world, global_gui_camera.get_id(), global_viewport);
  }

  void shutdown(void)
  {
    if (DISPLAY_SAMPLE_DESC)
      world::despawn_text(global_gui_world, main_description);

    world::despawn_text(global_gui_world, description);

    if (samples[sample_index].shutdown)
      samples[sample_index].shutdown();

    global_game_camera.shutdown();
    global_gui_camera.shutdown();

    application::destroy_world(global_game_world);
    application::destroy_world(global_gui_world);

    resource_package::unload(global_simple_package);
    resource_package::unload(global_samp_ball_pkg);

    application::release_resource_package(global_simple_package);
    application::release_resource_package(global_samp_ball_pkg);
  }
}


void main_app(int argc, char * argv[])
{
  memory_globals::init();

  {
    Allocator *a = &memory_globals::default_allocator();

    char data_path[_MAX_PATH];

    if (argc == 1){
      char *sandbox_path = "../sandbox/src/data";
      strcpy(data_path, file_system::directory_exists(sandbox_path) ? sandbox_path : "data");
    }
    else
      strcpy(data_path, argv[1]);

    application::init(app::init, app::update, app::render, app::shutdown, NULL, NULL, NULL, render_end, NULL, data_path, *a);

    while (!application::should_quit())
      application::update();

    application::shutdown();
  }

  memory_globals::shutdown();
}

int main(int argc, char * argv[])
{
  crash_handler::handle(argc, argv, main_app);

  return 0;
}