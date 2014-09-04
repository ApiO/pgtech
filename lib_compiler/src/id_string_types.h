#pragma once

#include <runtime/memory_types.h>
#include <runtime/collection_types.h>
#include <runtime/string_stream.h>
#include "mysqlite.h"

namespace pge
{
  struct IdStringManager
  {
    IdStringManager(Allocator &a);

    Allocator *_allocator;
    sqlite3   *_db;
    sqlite3_stmt *_stmt_lookup;
    string_stream::Buffer _sqlbuf;
    Hash<char*> _id_strings;
    Array<u32>  _to_save;
  };
}