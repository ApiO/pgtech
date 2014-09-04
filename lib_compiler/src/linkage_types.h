#pragma once

#include "runtime/memory_types.h"
#include "runtime/string_stream.h"

#include "data/types.h"

#include "mysqlite.h"

namespace pge
{
  struct LinkageManager
  {
    LinkageManager(Allocator &a) : 
      _allocator(&a), _sqlbuf(a), _dependencies(a), _dependents(a), _references(a), _changes(a) {};

    Allocator *_allocator;
    sqlite3   *_db;
    string_stream::Buffer _sqlbuf;
    char       _dir[MAX_PATH];
    Hash<u64>  _dependencies;
    Hash<u64>  _dependents;
    Hash<u64>  _references;

    enum Change {
      CHANGE_UPDATE,
      CHANGE_DELETE
    };

    // db batch buffers
    Hash<Change> _changes; // changes of resource dependencies
  };
}