#pragma once

#include <runtime/types.h>
#include <runtime/collection_types.h>
#include <runtime/json.h>

#include <data/types.h>
#include <data/data_manager.h>

#include "linkage_types.h"
#include "id_string_types.h"

namespace pge
{
  struct Metadata
  {
    u64 mtime;
    u64 size;
  };

  enum State
  {
    STATE_CREATED,
    STATE_UPDATED,
    STATE_DELETED,
    STATE_COMPILE_SUCCESS,
    STATE_COMPILE_ERROR
  };

  struct Project
  {
    Project(Allocator &a, StringPool &sp) :
      allocator(&a), 
      string_pool(&sp),
      metadata(a),
      data_ctx(a),
      linkage_ctx(a), 
      id_string_ctx(a),
      watched(false),
      first_scan(true),
      file_listener(0u) {};

    Allocator        *allocator;
    StringPool       *string_pool;
    DataManager       data_ctx;
    sqlite3          *db;
    LinkageManager    linkage_ctx;
    IdStringManager   id_string_ctx;
    char              src_dir[MAX_PATH], data_dir[MAX_PATH];
    Hash<Metadata>    metadata;
    Json             *compilers_config;
    Json             *settings_config;
    bool              first_scan;
    bool              watched;
    u64               file_listener;
  };

  struct Work
  {
    DataId   id;
    u32      state;
    Project *project;
    char    *src;
    FILE    *data;
  };
}