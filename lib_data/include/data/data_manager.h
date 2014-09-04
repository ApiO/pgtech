#pragma once

#include <Windows.h>
#include <stdio.h>
#include <mysqlite.h>

#include <runtime/types.h>
#include <runtime/memory_types.h>
#include <runtime/string_stream.h>
#include <runtime/hash.h>

#include <data/types.h>

namespace pge
{
  struct DataManager
  {
    enum ChangeType
    {
      CHANGE_UPDATE,
      CHANGE_DELETE
    };

    struct Change
    {
      u32 type;
      u32 file;
      u32 msize; // memory resident part size
      u32 ssize; // streaming part size
    };

    DataManager(Allocator &a) : allocator(&a), changes(a), sqlbuf(a) {};
    Allocator    *allocator;
    sqlite3      *db;
    sqlite3_stmt *stmt_get;
    string_stream::Buffer sqlbuf;
    Hash<Change>  changes;
    char          dir[MAX_PATH];
    u32           last_file;
  };

  namespace data_manager
  {
    void startup      (DataManager &ctx, const char *data_dir, sqlite3 *db);

    // opens the memory resident part of the specified resource
    // returns NULL if the resource does not extist
    FILE *open_read   (DataManager &ctx, const DataId &id, u32 &size);

    // opens the streaming part of the specified resource
    // returns NULL if the resource does not extist
    FILE *open_stream (DataManager &ctx, const DataId &id, u32 &size);

    // creates and opens a data file for the specified resource id
    // if the resource already has an associated data file, a new one is created
    FILE *open_write  (DataManager &ctx, const DataId &id);

    // closes a previously opened data file and sets its metadata
    void  close_write (DataManager &ctx, const DataId &id, FILE *file, u32 memory_size, u32 stream_size);

    // deletes a resource data file
    void  remove      (DataManager &ctx, const DataId &id);

    // save resource/data file mapping changes in the db
    void  save        (DataManager &ctx);

    // physically delete unused data file
    void  clean       (DataManager &ctx);

    // clean & save before to shutdown the system
    void  shutdown    (DataManager &ctx);
  }
}