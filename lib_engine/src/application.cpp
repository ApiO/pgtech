#include <runtime/types.h>
#include <runtime/json.h>
#include <runtime/tinycthread.h>
#include <runtime/trace.h>
#include <runtime/memory.h>
#include <runtime/timer.h>
#include <runtime/idlut.h>

#include <engine/pge.h>
#include "application.h"
#include "resource_package.h"
#include "physics/actor_resource.h"
#include "physics/shape_resource.h"
#include "audio/sound_resource.h"
#include "sprite/sprite_resource.h"
#include "text/font_resource.h"
#include "physics/physics_system.h"
#include "geometry/geometric_system.h"
#include "renderer/renderer.h"
#include "audio/audio_system.h"
#include "text/text_system.h"
#include "sprite/sprite_system.h"
#include "particle/particle_system.h"
#include "scene/scene_system.h"
#include "camera/camera_system.h"
#include "window/window.h"
#include "input_system.h"
#include "culling/culling_system.h"

#include "physics/physics_debug.h"
#include "culling/culling_debug.h"

#if CHRONO_STEPS
#include <utils/app_watcher.h>
#endif

#ifdef _TTHREAD_POSIX_
#define Sleep(x) usleep((x)*1000)
#endif

// UPDATE THREAD

namespace
{
  using namespace pge;

  static int update_thread_func(void *arg)
  {
    (Application*)arg;

    mint_store_32_relaxed(&app->end, 1u);

    while (mint_load_32_relaxed(&app->should_quit) == 0u)
    {
      while (mint_load_32_relaxed(&app->begin) == 0u &&
        mint_load_32_relaxed(&app->should_quit) == 0u)
      {
        thrd_yield();
      };

#if CHRONO_STEPS
      Timer(timer_cb);
      Timer(update_timer);
      start_timer(timer_cb);
      start_timer(update_timer);
#endif

      if (mint_load_32_relaxed(&app->should_quit) == 1u) break;
      mint_store_32_relaxed(&app->begin, 0u);

      // call update callback
      app->cb_update(app->delta_time);

#if CHRONO_STEPS
      stop_timer(timer_cb);
      app_watcher::save_value(WL_GAMEPLAY_UPDATE_CB, get_elapsed_time_in_ms(timer_cb));
      start_timer(timer_cb);
#endif

      // call render callback
      app->cb_render();

#if CHRONO_STEPS
      stop_timer(timer_cb);
      app_watcher::save_value(WL_GAMEPLAY_RENDER_CB, get_elapsed_time_in_ms(timer_cb));
#endif

      mint_store_32_relaxed(&app->end, 1u);

#if CHRONO_STEPS
      stop_timer(update_timer);
      app_watcher::save_value(WL_UPDATE_THREAD, get_elapsed_time_in_ms(update_timer));
#endif
    }
    return 0;
  }

  inline void update_begin(void)
  {
    mint_store_32_relaxed(&app->begin, 1u);
  }

  inline void wait_update_end(void)
  {
    while (mint_load_32_relaxed(&app->should_quit) == 0u &&
      mint_load_32_relaxed(&app->end) == 0u)
    {
      thrd_yield();
    };

    mint_store_32_relaxed(&app->end, 0u);
  }

  inline void shutdown_update_thread()
  {
    int result;
    thrd_join(app->update_thread, &result);
  }

  inline void initialize_update_thread()
  {
    thrd_create(&app->update_thread, update_thread_func, app);

    while (mint_load_32_relaxed(&app->end) == 0u)
    {
      thrd_yield();
    };

    mint_store_32_relaxed(&app->end, 0u);
  }

  inline void unload_queued_resources()
  {
    for (u32 i = 0; i < array::size(app->resource_unload_queue); i++)
      resource_manager::unload(app->resource_unload_queue[i].type, app->resource_unload_queue[i].name);
    array::clear(app->resource_unload_queue);
  }
}

// INTERNALS

namespace
{
  const f32 FRAME_DURATION = 1.f / 60.f;

  const char *SETTINGS_FILE_NAME = "settings.pgconf";

  inline void handle_fps(void)
  {
    const f64 wait_threshold = (f64)FRAME_DURATION / 100 * 1000; // in micro secs
    f64 remaining; // in micro nanosecs
    f64 cur_time = window::get_time();
    f64 delta_time = cur_time - app->last_frame;

    if (app->fps_control)
    {
      // wait if we're too fast
      while (delta_time < FRAME_DURATION) {
        remaining = (FRAME_DURATION - delta_time) * 1000;
        if (remaining > wait_threshold)
          Sleep(DWORD(remaining - wait_threshold));

        cur_time = window::get_time();
        delta_time = cur_time - app->last_frame;
      }
      // avoid spiral of death
      if (delta_time > .05f) delta_time = .05f;
    }

    app->delta_time = delta_time;
    app->last_frame = cur_time;

#if CHRONO_STEPS
    app_watcher::save_value(WL_DELTA_TIME, delta_time * 1000);
#endif
  }
}

// c++ API

namespace pge
{
  Application *app;

  namespace application
  {
    u64 resource_package(const char *name)
    {
      const u32 package_name = murmur_hash_32(name);
      resource_manager::load(RESOURCE_TYPE_PACKAGE, package_name);
      return package_name;
    }

    void release_resource_package(u64 package)
    {
      resource_manager::unload(RESOURCE_TYPE_PACKAGE, (u32)package);
    }

    void init(Init _init, Update update, Render render, Shutdown shutdown, Synchro synchro,
      RenderInit render_init, RenderBegin render_begin, RenderEnd render_end, RenderShutdown render_shutdown,
      const char *data_dir, Allocator &a, bool create_window)
    {
#if CHRONO_STEPS
      app_watcher::init(5000, a);

      Timer(timer);
      start_timer(timer);
#endif
      StringPool sp(a);
      Json setting_file(a, sp);
      char tmp[160];
      sprintf(tmp, "%s\\%s", data_dir, SETTINGS_FILE_NAME);

      bool r = json::parse_from_file(setting_file, json::root(setting_file), tmp);
      XASSERT(r, "File not found : \"%s\"", tmp);

      app = MAKE_NEW(a, Application, a);

      // set the callbacks
      app->cb_init     = _init;
      app->cb_update   = update;
      app->cb_render   = render;
      app->cb_shutdown = shutdown;
      app->cb_synchro  = synchro;
      app->fps_control = true;

      resource_manager::init(data_dir, a);
      sound_resource::register_type();
      sprite_resource::register_type();
      font_resource::register_type();
      actor_resource::register_type();
      shape_resource::register_type();

      // Loads boot package
      const Json::Node &boot = json::get_node(setting_file, json::root(setting_file), "boot");

      const char *res_name = json::get_string(setting_file, boot.id, "package");
      app->boot_package = resource_package(res_name);
      resource_package::load(app->boot_package);
      resource_package::flush(app->boot_package);

      // Loads physics
      res_name = json::get_string(setting_file, boot.id, "physics");
      physics_system::init(murmur_hash_32(res_name), a);

      // Setups audio system
      res_name = json::get_string(setting_file, boot.id, "audio");
      audio_system::init(murmur_hash_32(res_name), a);

      // initialize the window
      window::initialize();
      if (create_window) {
        const Json::Node &window = json::get_node(setting_file, json::root(setting_file), "window");
        const Json::Node &resolution = json::get_node(setting_file, window.id, "resolution");
        bool full_screen = strcmp(json::get_string(setting_file, window.id, "mode", "WINDOW"), "WINDOW") != 0;
        bool resizable = json::get_bool(setting_file, window.id, "resizable");
        window::create(
          json::get_string(setting_file, window.id, "title"),
          json::get_integer(setting_file, resolution.id, 0),
          json::get_integer(setting_file, resolution.id, 1),
          full_screen, resizable);
      }
      
#if CHRONO_STEPS
      Timer(timer_cb);
      start_timer(timer_cb);
#endif

      // call the init callback
      app->cb_init();

#if CHRONO_STEPS
      stop_timer(timer_cb);
      app_watcher::save_value(WL_GAMEPLAY_INIT_CB, get_elapsed_time_in_ms(timer_cb));
#endif

      // initialize application variables
      app->time_start = window::get_time();
      app->last_frame = app->time_start;
      app->frame_index = 0llu;
      app->begin._nonatomic = 0u;
      app->end._nonatomic = 0u;
      app->should_quit._nonatomic = 0u;
      mint_thread_fence_release();

      // initialize rendering
      const char *sprite_program    = json::get_string(setting_file, boot.id, "sprite_program");
      const char *primitive_program = json::get_string(setting_file, boot.id, "primitive_program");
      const char *particle_program  = json::get_string(setting_file, boot.id, "particle_program");
      renderer::init(murmur_hash_32(sprite_program), murmur_hash_32(primitive_program), murmur_hash_32(particle_program)
        , render_init, render_begin, render_end, render_shutdown, a);


      initialize_update_thread();

#if CHRONO_STEPS
      stop_timer(timer);
      app_watcher::save_value(WL_APP_INIT, get_elapsed_time_in_ms(timer));
#endif
    }

    void init(Init _init, Update update, Render render, Shutdown shutdown,
      const char *data_dir, Allocator &a, bool create_window)
    {
      init(_init, update, render, shutdown, NULL,
        NULL, NULL, NULL, NULL,
        data_dir, a, create_window);
    }

    bool should_quit(void)
    {
      return mint_load_32_relaxed(&app->should_quit) == 1u;
    }

    void quit(void)
    {
      mint_store_32_relaxed(&app->should_quit, 1u);
      shutdown_update_thread();
    }

    void update(void)
    {
#if CHRONO_STEPS
      Timer(frame_timer);
      start_timer(frame_timer);
#endif
      unload_queued_resources();
      handle_fps();
      window::poll_events();

      if (should_quit() || window::should_close()){
        quit();
        return;
      }

      input_system::update();   // update the input states
      update_begin();           // start update thread task
      renderer::render();       // render the previous frame
      wait_update_end();        // synchronizes threads
      renderer::swap_buffers(); // buffers swap

      if (app->cb_synchro) {
        app->cb_synchro();
        if (should_quit()) return;
      }

      app->frame_index++;

#if CHRONO_STEPS
      stop_timer(frame_timer);
      app_watcher::save_value(WL_MAIN_LOOP, get_elapsed_time_in_ms(frame_timer));
#endif
  }

    void shutdown(void)
    {
      // call the shutdown callback
      app->cb_shutdown();
      unload_queued_resources();

      audio_system::shutdown();
      renderer::shutdown();
      physics_system::shutdown();
      window::shutdown();

#if CHRONO_STEPS
      app_watcher::shut_down();
#endif

      resource_package::unload(app->boot_package);
      release_resource_package(app->boot_package);

      unload_queued_resources(); // unload the resources of the boot package
      resource_manager::shutdown();

      MAKE_DELETE((*app->_a), Application, app);
    }

    u64 create_viewport(u32 x, u32 y, u32 width, u32 height)
    {
      Viewport vp;
      vp.screen_x = x;
      vp.screen_y = y;
      vp.width = width;
      vp.height = height;

      return idlut::add(app->viewports, vp);
    }

    void set_viewport(u64 viewport, u32 x, u32 y, u32 width, u32 height)
    {
      Viewport &vp = *idlut::lookup(app->viewports, viewport);
      vp.height = height;
      vp.width = width;
      vp.screen_x = x;
      vp.screen_y = y;
    }

    void destroy_viewport(u64 viewport)
    {
      idlut::remove(app->viewports, viewport);
    }


    u64 create_world()
    {
      World *w = MAKE_NEW(*app->_a, World, *app->_a);
      const u64 id = idlut::add(app->worlds, w);
      physics_system::set_gravity(w->physics_world, glm::vec3(0, -10, 0));
      particle_system::init(w->particle_system);
      return id;
    }

    void destroy_world(u64 world)
    {
      World *w = *idlut::lookup(app->worlds, world);

      particle_system::shutdown(w->particle_system);
      MAKE_DELETE(*app->_a, World, w);
      idlut::remove(app->worlds, world);
    }


    void get_worlds(Array<u64> &worlds)
    {
      array::resize(worlds, 0);
      array::reserve(worlds, idlut::size(app->worlds));
      IdLookupTable<World*>::Entry *e, *end = idlut::end(app->worlds);
      for (e = idlut::begin(app->worlds); e < end; e++)
        array::push_back(worlds, e->id);
    }

#if CHRONO_STEPS
    Timer(frame_timer);
    start_timer(frame_timer);
#endif

    void render_line(const glm::vec3 &a, const glm::vec3 &b, const Color &color, u64 world, u64 camera, u64 viewport)
    {
      Viewport &v = *idlut::lookup(app->viewports, viewport);
      World &w = **idlut::lookup(app->worlds, world);

      renderer::start_batch(v.screen_x, v.screen_y, v.width, v.height,
        idlut::lookup(w.camera_system, camera)->projection_view);

      geometric_system::gather_line(a, b, color);
    }

    void render_polygon(const glm::vec3 *vertices, u32 num_vertices, const Color &color, u64 world, u64 camera, u64 viewport)
    {
      Viewport &v = *idlut::lookup(app->viewports, viewport);
      World &w = **idlut::lookup(app->worlds, world);

      renderer::start_batch(v.screen_x, v.screen_y, v.width, v.height,
        idlut::lookup(w.camera_system, camera)->projection_view);

      geometric_system::gather_polygon(vertices, num_vertices, color);
    }

    void render_circle(const glm::vec3 &center, f32 radius, const Color &color, u64 world, u64 camera, u64 viewport, bool surface)
    {
      Viewport &v = *idlut::lookup(app->viewports, viewport);
      World &w = **idlut::lookup(app->worlds, world);

      renderer::start_batch(v.screen_x, v.screen_y, v.width, v.height,
        idlut::lookup(w.camera_system, camera)->projection_view);

      geometric_system::gather_circle(center, radius, color, surface);
    }

    void render_world(u64 world, u64 camera, u64 viewport)
    {
      Viewport &v =  *idlut::lookup(app->viewports, viewport);
      World    &w = **idlut::lookup(app->worlds, world);
      Camera &cam =  *idlut::lookup(w.camera_system, camera);

      renderer::start_batch(v.screen_x, v.screen_y, v.width, v.height, cam.projection_view);

#if CHRONO_STEPS
      Timer(frame_timer);
      start_timer(frame_timer);
#endif

      // CULLING
      culling_system::cull(w.culling_system, cam.frustum);
      sprite_system::gather(w.sprite_system);
      text_system::gather(w.text_system);
      geometric_system::gather(w.geometric_system);
      particle_system::gather(w.particle_system);
      physics_debug::gather(w.physics_world);
      culling_debug::gather(w.culling_system);

#if CHRONO_STEPS
      stop_timer(frame_timer);
      app_watcher::save_value(WL_RENDER_WORLD, get_elapsed_time_in_ms(frame_timer));
#endif
    }

    static inline bool plane3_inter(const Plane &p1, const Plane &p2, const Plane &p3, glm::vec3 &result) {
      glm::vec3 n1 = glm::vec3(p1.a, p1.b, p1.c);
      glm::vec3 n2 = glm::vec3(p2.a, p2.b, p2.c);
      glm::vec3 n3 = glm::vec3(p3.a, p3.b, p3.c);
      f32 denom = glm::dot(n1, glm::cross(n2, n3));

      if (fabsf(denom) < FLT_EPSILON)
        return false;

      result = (glm::cross(n2, n3)*p1.d +
        glm::cross(n3, n1)*p2.d +
        glm::cross(n1, n2)*p3.d) / -denom;
      return true;
    }

    void render_frustum(u64 camera, u64 world, u64 render_camera, u64 viewport)
    {
      World    &w = **idlut::lookup(app->worlds, world);
      Camera   &cam = *idlut::lookup(w.camera_system, camera);
      Camera   &rcam = *idlut::lookup(w.camera_system, render_camera);
      Viewport &v = *idlut::lookup(app->viewports, viewport);

      glm::vec3 points[8];
      bool valid = true;

      valid &= plane3_inter(cam.frustum.left, cam.frustum.bottom, cam.frustum._near, points[0]);
      valid &= plane3_inter(cam.frustum.right, cam.frustum.bottom, cam.frustum._near, points[1]);
      valid &= plane3_inter(cam.frustum.right, cam.frustum.top, cam.frustum._near, points[2]);
      valid &= plane3_inter(cam.frustum.left, cam.frustum.top, cam.frustum._near, points[3]);
      valid &= plane3_inter(cam.frustum.left, cam.frustum.bottom, cam.frustum._far, points[4]);
      valid &= plane3_inter(cam.frustum.right, cam.frustum.bottom, cam.frustum._far, points[5]);
      valid &= plane3_inter(cam.frustum.right, cam.frustum.top, cam.frustum._far, points[6]);
      valid &= plane3_inter(cam.frustum.left, cam.frustum.top, cam.frustum._far, points[7]);

      XASSERT(valid, "Invalid frustum.");

      const Color color = Color(0, 255, 0, 255);

      renderer::start_batch(v.screen_x, v.screen_y, v.width, v.height, rcam.projection_view);

      geometric_system::gather_polygon(points, 4, color);
      geometric_system::gather_polygon(points + 4, 4, color);

      geometric_system::gather_line(points[0], points[4], color);
      geometric_system::gather_line(points[1], points[5], color);
      geometric_system::gather_line(points[2], points[6], color);
      geometric_system::gather_line(points[3], points[7], color);
    }

    void set_autoload(bool enabled)
    {
      resource_manager::set_autoload(enabled);
    }

    void show_culling_debug(bool value)
    {
      culling_debug::show(value);
    }
}
}