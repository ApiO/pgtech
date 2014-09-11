#include <runtime/memory.h>
#include <runtime/idlut.h>
#include <runtime/array.h>
#include <data/data_manager.h>
#include <mintomic/mintomic.h>
#include <runtime/tinycthread.h>
#include <runtime/temp_allocator.h>

#include "resource_manager.h"

#ifdef _TTHREAD_POSIX_
#define Sleep(x) usleep((x)*1000)
#endif

// -------------------------------------------------------------------------
// Internals
// -------------------------------------------------------------------------

namespace
{
  using namespace pge;

  struct TypeCallbacks
  {
    ResourceLoad     load;
    ResourceBringIn  bring_in;
    ResourcePatchUp  patch_up;
    ResourceBringOut bring_out;
    ResourceUnload   unload;
  };

  const TypeCallbacks DEFAULT_CALLBACKS =
  {
    resource_manager::default_load,
    resource_manager::default_bring_in,
    resource_manager::default_patch_up,
    resource_manager::default_bring_out,
    resource_manager::default_unload,
  };

  struct ResourceEntry
  {
    void *data;
    i32   references;
  };

  struct ResourceManager
  {
    ResourceManager(Allocator &a) :
      map(a), types(a), data_ctx(a), loader_front_buffer(a), loader_back_buffer(a) {}

    struct Command {
      bool   load; // unload if false
      DataId id;
    };

    Hash<ResourceEntry> map;
    Hash<TypeCallbacks> types;
    DataManager         data_ctx;
    thrd_t              loader_thread;
    Array<Command>      loader_front_buffer;
    Array<Command>      loader_back_buffer;
    mtx_t               loader_mutex;
    mint_atomic32_t     loader_shutdown;
    mtx_t               map_mutex;
    cnd_t               loader_wait;

    bool autoload;
  };

  char _buffer[sizeof(ResourceManager)];
  ResourceManager *_rmng;

  void load(const DataId &id)
  {
    ResourceEntry re ={ 0 };
    u32 size = 0;
    FILE *f = data_manager::open_read(_rmng->data_ctx, id, size);

    mtx_lock(&_rmng->map_mutex);
    re = hash::get(_rmng->map, id.as64, re);
    mtx_unlock(&_rmng->map_mutex);

    ASSERT(re.data == 0);
    re.references = 1;
    re.data = hash::get(_rmng->types, id.fields.type, DEFAULT_CALLBACKS).load(f, id.fields.name, size);
    fclose(f);

    mtx_lock(&_rmng->map_mutex);
    hash::set(_rmng->map, id.as64, re);
    mtx_unlock(&_rmng->map_mutex);
  }

  void unload(const DataId &id)
  {
    ResourceEntry re ={ 0 };
    mtx_lock(&_rmng->map_mutex);
    re = hash::get(_rmng->map, id.as64, re);
    mtx_unlock(&_rmng->map_mutex);

    ASSERT(re.data != 0);
    hash::get(_rmng->types, id.fields.type, DEFAULT_CALLBACKS).unload(re.data, id.fields.name);

    mtx_lock(&_rmng->map_mutex);
    hash::remove(_rmng->map, id.as64);
    mtx_unlock(&_rmng->map_mutex);
  }

  int loader_update(void *arg) {
    (void)arg;
    Array<ResourceManager::Command> &back_buf = _rmng->loader_back_buffer;
    bool run = true;

    while (run) {
      run = (mint_load_32_relaxed(&_rmng->loader_shutdown) == 0);
      mtx_lock(&_rmng->loader_mutex);
      array::copy(back_buf, _rmng->loader_front_buffer);
      array::clear(_rmng->loader_front_buffer);
      mtx_unlock(&_rmng->loader_mutex);

      if (!array::size(back_buf)) {
        Sleep(1);
        continue;
      }

      for (u32 i = 0; i < array::size(back_buf); i++) {
        if (back_buf[i].load)
          load(back_buf[i].id);
        else
          unload(back_buf[i].id);
      }
    }
    return 0;
  }
}

// -------------------------------------------------------------------------
// Default Callbacks
// -------------------------------------------------------------------------

namespace pge
{
  namespace resource_manager
  {
    void *default_load(FILE *file, u32 name, u32 size)
    {
      (void)name;
      void *resource = memory_globals::default_allocator().allocate(size);
      fread(resource, size, 1, file);

      return resource;
    }

    void default_unload(void *data, u32 name) {
      (void)name;
      memory_globals::default_allocator().deallocate(data);
    }

    void default_bring_in(void *data, u32 name)  { (void)data, name; }
    void default_patch_up(void *data, u32 name)  { (void)data, name; }
    void default_bring_out(void *data, u32 name) { (void)data, name; }
  }
}

// -------------------------------------------------------------------------
// Manager
// -------------------------------------------------------------------------

namespace pge
{
  namespace resource_manager
  {
    void init(const char *data_dir, Allocator &a)
    {
      char szdb[MAX_PATH];
      sqlite3 *db;

      _rmng = new (_buffer)ResourceManager(a);
      _rmng->autoload = false;

      sprintf(szdb, "%s\\%s", data_dir, "build.db");
      sqlite3_check(sqlite3_open(szdb, &db));

      data_manager::startup(_rmng->data_ctx, data_dir, db);
      _rmng->loader_shutdown._nonatomic = 0;
      mtx_init(&_rmng->loader_mutex, mtx_plain);
      mtx_init(&_rmng->map_mutex, mtx_plain);
      thrd_create(&_rmng->loader_thread, loader_update, _rmng);
    }

    void shutdown()
    {
      data_manager::shutdown(_rmng->data_ctx);
      sqlite3_check(sqlite3_close(_rmng->data_ctx.db));

      TempAllocator1024 ta;
      Array<DataId> unload_queue(ta);

      const Hash<ResourceEntry>::Entry *e, *end = hash::end(_rmng->map);
      DataId id;

      mtx_lock(&_rmng->map_mutex);
      for (e = hash::begin(_rmng->map); e < end; e++) {
        id.as64 = e->key;
        array::push_back(unload_queue, id);
      }
      mtx_unlock(&_rmng->map_mutex);

      for (u32 i = 0; i < array::size(unload_queue); i++)
        unload((ResourceType)unload_queue[i].fields.type, unload_queue[i].fields.name);

      int r;
      mint_store_32_relaxed(&_rmng->loader_shutdown, 1);
      thrd_join(_rmng->loader_thread, &r);
      mtx_destroy(&_rmng->loader_mutex);
      mtx_destroy(&_rmng->map_mutex);

      _rmng->~ResourceManager();
    }

    void register_type(ResourceType type,
                       ResourceLoad load,
                       ResourceBringIn bring_in,
                       ResourcePatchUp patch_up,
                       ResourceBringOut bring_out,
                       ResourceUnload unload)
    {
      TypeCallbacks t;
      t.load      = load;
      t.bring_in  = bring_in;
      t.patch_up  = patch_up;
      t.bring_out = bring_out;
      t.unload    = unload;
      hash::set(_rmng->types, type, t);
    }

    void set_autoload(bool enabled)
    {
      _rmng->autoload = enabled;
    }

    void load(ResourceType type, u32 name)
    {
      ResourceManager::Command cmd;
      ResourceEntry re ={ 0 };
      cmd.load = true;
      cmd.id.fields.type = (u32)type;
      cmd.id.fields.name = name;

      mtx_lock(&_rmng->map_mutex);
      re = hash::get(_rmng->map, cmd.id.as64, re);
      ++re.references;
      hash::set(_rmng->map, cmd.id.as64, re);
      mtx_unlock(&_rmng->map_mutex);

      if (re.references == 1) {
        mtx_lock(&_rmng->loader_mutex);
        array::push_back(_rmng->loader_front_buffer, cmd);
        mtx_unlock(&_rmng->loader_mutex);
      }
    }

    void unload(ResourceType type, u32 name)
    {
      ResourceManager::Command cmd;
      ResourceEntry re ={ 0 };

      cmd.load = false;
      cmd.id.fields.type = (u32)type;
      cmd.id.fields.name = name;

      mtx_lock(&_rmng->map_mutex);
      re = hash::get(_rmng->map, cmd.id.as64, re);
      --re.references;
      hash::set(_rmng->map, cmd.id.as64, re);
      mtx_unlock(&_rmng->map_mutex);

      if (re.references == 0) {
        hash::get(_rmng->types, cmd.id.fields.type, DEFAULT_CALLBACKS).bring_out(re.data, name);
        mtx_lock(&_rmng->loader_mutex);
        array::push_back(_rmng->loader_front_buffer, cmd);
        mtx_unlock(&_rmng->loader_mutex);
      }
    }

    bool has_loaded(ResourceType type, u32 name)
    {
      DataId id;
      ResourceEntry re ={ 0 };
      id.fields.type = type;
      id.fields.name = name;

      mtx_lock(&_rmng->loader_mutex);
      re = hash::get(_rmng->map, id.as64, re);
      mtx_unlock(&_rmng->loader_mutex);
      return re.data != NULL;
    }

    void flush(ResourceType type, u32 name)
    {
      DataId id;
      ResourceEntry e ={ 0 };
      id.fields.type = type;
      id.fields.name = name;

      // wait for the package to load
      while (!has_loaded(type, name))
        thrd_yield();

      mtx_lock(&_rmng->map_mutex);
      ASSERT(hash::has(_rmng->map, id.as64));
      e = hash::get(_rmng->map, id.as64, e);
      mtx_unlock(&_rmng->map_mutex);

      if (e.references == 1)
        hash::get(_rmng->types, id.fields.type, DEFAULT_CALLBACKS).bring_in(e.data, name);
    }

    void patch_up(ResourceType type, u32 name)
    {
      DataId id;
      id.fields.type = (u32)type;
      id.fields.name = name;

      const ResourceEntry e ={ 0 };
      void *data = hash::get(_rmng->map, id.as64, e).data;

      hash::get(_rmng->types, id.fields.type, DEFAULT_CALLBACKS).patch_up(data, name);
    }

    void *get(ResourceType type, u32 name)
    {
      DataId id;
      id.fields.type = type;
      id.fields.name = name;
      const ResourceEntry e ={ 0 };
      mtx_lock(&_rmng->map_mutex);
      void *r = hash::get(_rmng->map, id.as64, e).data;
      mtx_unlock(&_rmng->map_mutex);
      if (_rmng->autoload && !r) {
        load(type, name);
        flush(type, name);
        patch_up(type, name);
        r = hash::get(_rmng->map, id.as64, e).data;
      }
      XASSERT(r, "Resource \"%u\", type: %u not found.", name, (u32)type);
      return r;
    }

    FILE *open_stream(ResourceType type, u32 name)
    {
      DataId id;
      u32    size;
      id.fields.type = type;
      id.fields.name = name;
      return data_manager::open_stream(_rmng->data_ctx, id, size);
    }

    u32 name(void *resource)
    {
      DataId id;
      id.fields.name = 0;

      const Hash<ResourceEntry>::Entry *e, *end = hash::end(_rmng->map);
      for (e = hash::begin(_rmng->map); e < end; e++) {
        if (e->value.data == resource) {
          id.as64 = e->key;
          return id.fields.name;
        }
      }
      return id.fields.name;
    }
  }
}