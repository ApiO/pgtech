
#include <windows.h>
#include <mutex>

#include <runtime/idlut.h>
#include <runtime/assert.h>
#include <runtime/tinycthread.h>
#include <time.h>

#include "file_watcher.h"

namespace
{
  using namespace std;
  using namespace pge;
  using namespace pge::file_system;

  struct EventInfo
  {
    time_t stamp;
    char   file_name[MAX_PATH];
    WatchEventType type;
    bool operator ==(EventInfo &other){
      return (stamp == other.stamp && type == other.type && strcmp(file_name, other.file_name) == 0);
    }
  };

  struct FileListener
  {
    FileListener(Allocator &a)
      : events(a), quit(false), dir_name(NULL)
    {
      mtx_init(&mtx, mtx_plain);
    }
    ~FileListener()
    {
      mtx_destroy(&mtx);
    }
    OVERLAPPED   overlapped;
    BYTE         buffer[32 * 1024];
    HANDLE       dir_handle;
    DWORD        notify_filter;
    char        *dir_name;
    bool         quit;
    bool         recursive;
    mtx_t        mtx;
    thrd_t       thread;
    Array<WatchEvent> events;
    EventInfo    last_event;
  };

  bool refresh_watch(FileListener &listener, bool clear = false);

  void CALLBACK cb_watch(DWORD error, DWORD num_bytes, LPOVERLAPPED lpOverlapped)
  {
    PFILE_NOTIFY_INFORMATION notify;
    FileListener &listener = *(FileListener*)lpOverlapped;
    size_t offset = 0;

    EventInfo event_info;
    time(&event_info.stamp); 

    if (num_bytes == 0)
      return;

    if (error == ERROR_SUCCESS)
    {
      do
      {
        notify = (PFILE_NOTIFY_INFORMATION)&listener.buffer[offset];
        offset += notify->NextEntryOffset;

#			if defined(UNICODE)
        {
          lstrcpynW(szFile, notify->FileName,
                    min(MAX_PATH, notify->FileNameLength / sizeof(WCHAR)+1));
        }
#			else
        {
          int count = WideCharToMultiByte(CP_ACP, 0, notify->FileName,
                                          notify->FileNameLength / sizeof(WCHAR),
                                          event_info.file_name, MAX_PATH - 1, NULL, NULL);
          event_info.file_name[count] = TEXT('\0');
        }
#			endif

        switch (notify->Action)
        {
          case FILE_ACTION_RENAMED_NEW_NAME:
          case FILE_ACTION_ADDED:
            event_info.type = WATCH_ADDED;
            break;
          case FILE_ACTION_RENAMED_OLD_NAME:
          case FILE_ACTION_REMOVED:
            event_info.type  = WATCH_REMOVED;
            break;
          case FILE_ACTION_MODIFIED:
            event_info.type  = WATCH_UPDATED;
            break;
        };

        if (listener.last_event == event_info) continue;
        listener.last_event = event_info;

        mtx_lock(&listener.mtx);
        
        WatchEvent e(event_info.type, listener.dir_name, event_info.file_name, listener.events._allocator);
        array::push_back(listener.events, e);

        mtx_unlock(&listener.mtx);

      } while (notify->NextEntryOffset != 0);
    }

    /*
    
    listener.mtx.lock();
    if (!listener.quit)
      refresh_watch(listener);
    
    listener.mtx.unlock();

    */
  }

  bool refresh_watch(FileListener &listener, bool clear)
  {
    return ReadDirectoryChangesW(
      listener.dir_handle, listener.buffer, sizeof(listener.buffer), listener.recursive,
      listener.notify_filter, NULL, &listener.overlapped, clear ? 0 : cb_watch) != 0;
  }

  int listener_thread_func(void *arg)
  {
    FileListener &listener = *(FileListener*)arg;

    DWORD wait_result, bytes;
    bool run = true;

    while (run)
    {
      wait_result = MsgWaitForMultipleObjectsEx(1, &listener.dir_handle, 0, QS_ALLINPUT, MWMO_ALERTABLE);

      switch (wait_result)
      {
        case WAIT_ABANDONED_0:
        case WAIT_ABANDONED_0 + 1:
          //"Wait abandoned."
          break;
        case WAIT_TIMEOUT:
          break;
        case WAIT_FAILED:
          //"Wait failed."
          break;
        default:
            ReadDirectoryChangesW(
              listener.dir_handle, listener.buffer, sizeof(listener.buffer), listener.recursive,
              listener.notify_filter, NULL, &listener.overlapped, 0);
          if (GetOverlappedResult(listener.dir_handle, &listener.overlapped, &bytes, FALSE)){
            cb_watch(ERROR_SUCCESS, bytes, &listener.overlapped);
          }
      }

      Sleep(100);

      mtx_lock(&listener.mtx);
      run = !listener.quit;
      mtx_unlock(&listener.mtx);
    }
    return 0;
  }
}


namespace pge
{
  namespace file_system
  {
    u64 start_watch(FileWatcher &fw, const char *path, bool recursive)
    {
      XASSERT(directory_exists(path), "Directory not found : \"%s\"", path);

      FileListener &listener = *MAKE_NEW((*fw.a), FileListener, (*fw.a));

      listener.dir_handle = CreateFile(path,
                                       FILE_LIST_DIRECTORY,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                                       OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

      ASSERT(listener.dir_handle != INVALID_HANDLE_VALUE);

      listener.notify_filter = FILE_NOTIFY_CHANGE_CREATION |
        FILE_NOTIFY_CHANGE_LAST_WRITE |
        FILE_NOTIFY_CHANGE_FILE_NAME;

      //ASSERT(refresh_watch(listener));

      listener.recursive     = recursive;
      listener.dir_name      = (char*)fw.a->allocate(strlen(path)*sizeof(path));
      strcpy(listener.dir_name, path);

      u64 id = idlut::add(fw.listeners, (void*)&listener);

      thrd_create(&listener.thread, listener_thread_func, &listener);

      return id;
    }

    void stop_watch(FileWatcher &fw, u64 id)
    {
      FileListener &listener = *(FileListener*)*idlut::lookup(fw.listeners, id);

      mtx_lock(&listener.mtx);
      listener.quit = true;
      mtx_unlock(&listener.mtx);

      int result;
      thrd_join(listener.thread, &result);

      CancelIo(listener.dir_handle);

      //refresh_watch(listener, true);

      if (!HasOverlappedIoCompleted(&listener.overlapped))
        SleepEx(5, TRUE);

      ASSERT(CloseHandle(listener.overlapped.hEvent));
      ASSERT(CloseHandle(listener.dir_handle));

      fw.a->deallocate(listener.dir_name);

      MAKE_DELETE((*fw.a), FileListener, &listener);
    }

    void get_watch_events(FileWatcher &fw, u64 id, Array<WatchEvent> &events)
    {
      ASSERT(idlut::has(fw.listeners, id));
      FileListener &listener = *(FileListener*)*idlut::lookup(fw.listeners, id);

      mtx_lock(&listener.mtx);

      if (!array::size(listener.events)){
        mtx_unlock(&listener.mtx);
        return;
      }

      u32 size = array::size(listener.events);
      array::resize(events, size);

      Allocator *a = events._allocator;
      for (u32 i = 0; i < array::size(listener.events); i++)
      {
        WatchEvent &e = *(array::begin(listener.events) + i);
        events[i].type = e.type;
        events[i].dir = e.dir;
        events[i].filename = e.filename;
        events[i]._a = a;
      }

      array::clear(listener.events);
      mtx_unlock(&listener.mtx);
    }
  }
}