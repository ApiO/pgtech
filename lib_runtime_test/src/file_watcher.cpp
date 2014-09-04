#include <stdio.h>
#include <windows.h>

#include <runtime/types.h>
#include <runtime/memory.h>
#include <runtime/array.h>
#include <runtime/file_system.h>
#include <runtime/trace.h>

namespace pge
{
  void file_watcher_sample()
  {
    /*
    memory_globals::init();
    {
      Allocator &a = memory_globals::default_allocator();

      FileWatcher file_watcher(a);
      Array<WatchEvent> events(a);

      u64 listener = file_system::start_watch(file_watcher, "c:\\tmp", true);

      while (true)
      {
        file_system::get_watch_events(file_watcher, listener, events);

        WatchEvent *e = array::begin(events),
          *eend = array::end(events);

        // TODO: consommer le buff d'event
        for (; e < eend; e++)
          OUTPUT("ACTION type: %d\n\tdir: %s\n\tfilename: %s", e->type, e->dir, e->filename);

        Sleep(100);
      }

      file_system::stop_watch(file_watcher, listener);
    }
    memory_globals::shutdown();
    */
  }
}