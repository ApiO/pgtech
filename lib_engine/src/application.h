#pragma once

#include <mintomic/mintomic.h>
#include <runtime/tinycthread.h>

#include <runtime/idlut.h>
#include "application_types.h"

namespace pge
{
  typedef mint_atomic32_t mint32;

  struct Application
  {
    Application(Allocator &a);
    Init       cb_init;
    Update     cb_update;
    Render     cb_render;
    Shutdown   cb_shutdown;
    Synchro    cb_synchro;
    f64        time_start;
    f64        last_frame;
    f64        delta_time;
    u64        frame_index;
    void      *window;
    bool       fps_control;
    u64        boot_package;

    thrd_t     update_thread;
    mint32     begin;
    mint32     end;
    mint32     should_quit;

    IdLookupTable<World*>   worlds;
    IdLookupTable<Viewport> viewports;

    struct Resource {
      ResourceType type;
      u32          name;
    };
    Array<Resource> resource_unload_queue;

    Allocator *_a;
  };

  extern Application *app;

  namespace application
  {
    World &world(u64 world);
  }

  namespace application
  {
    inline World &world(u64 world) { return **idlut::lookup(app->worlds, world); }
  }

  inline Application::Application(Allocator &a) : _a(&a), worlds(a), viewports(a), window(NULL), resource_unload_queue(a){};
}