#include <string.h>

#include <runtime/memory.h>
#include <runtime/file_system.h>
#include <runtime/tinycthread.h>

namespace pge
{
  WatchEvent::WatchEvent(WatchEventType t, char *d, char *fn, Allocator *a)
    :type(t), dir(d), filename(fn), _a(a)
  {
    char *tmp = dir;
    dir = (char*)a->allocate(strlen(tmp)*sizeof(char));
    strcpy(dir, tmp);

    tmp = filename;
    filename = (char*)a->allocate(strlen(tmp)*sizeof(tmp));
    strcpy(filename, tmp);
  }

  WatchEvent::~WatchEvent()
  {
    _a->deallocate(dir);
    _a->deallocate(filename);
  }

  FileWatcher::FileWatcher(Allocator &a) : listeners(a), a(&a) {};

  FileWatcher::~FileWatcher(){
    IdLookupTable<void*>::Entry *e = idlut::begin(listeners),
      *eend = idlut::begin(listeners);
    for (; e < eend; e++)
      file_system::stop_watch(*this, e->id);
  };
}