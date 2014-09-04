#pragma once

#include <runtime/types.h>
#include <runtime/memory_types.h>
#include <runtime/collection_types.h>

namespace pge
{
  enum WatchEventType
  {
    WATCH_ADDED = 0,
    WATCH_UPDATED,
    WATCH_REMOVED
  };

  struct WatchEvent
  {
    WatchEvent(){}
    WatchEvent(WatchEventType t, char *d, char *fn, Allocator *a);
    ~WatchEvent();
    WatchEventType type;
    char          *dir;
    char          *filename;
    Allocator     *_a;
  };
  
  struct FileWatcher
  {
    FileWatcher(Allocator &a);
    ~FileWatcher();
    IdLookupTable<void*> listeners;
    Allocator *a;
  };

  namespace file_system
  {
    int  delete_directory         (const char *path, bool recursive = true);
    int  delete_directory_content (const char *path, bool recursive = true);
    bool directory_exists         (const char *path);
    bool directory_create         (const char *path);

    int  delete_file (const char *path);
    bool file_exists (const char *path);
    void copy_file   (const char *dest, const char *source);
   
    u64  start_watch(FileWatcher &fw, const char *path, bool recursive);
    void stop_watch(FileWatcher &fw, u64 listener);
    void get_watch_events(FileWatcher &fw, u64 listener, Array<WatchEvent> &events);
  }
}