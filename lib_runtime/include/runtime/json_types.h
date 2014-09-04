#pragma once

#include "runtime/collection_types.h"

namespace pge
{
  enum JsonType
  {
    JSON_NULL = 0,
    JSON_BOOLEAN,
    JSON_INTEGER,
    JSON_NUMBER,
    JSON_STRING,
    JSON_OBJECT,
    JSON_ARRAY
  };

  struct Json
  {
    Json  (Allocator &a, StringPool &sp);
    Json  (const Json &other, u64 root);
    ~Json ();

    struct Node {
      char *name;
      JsonType type;
      u64 id;
      union {
        bool  boolean;
        i32   integer;
        f64   number;
        char *string;
        u64   raw;
      } value;
      u64 prev, next, child;
    };

    IdLookupTable<Node> _nodes;
    u64          _root;
    Hash<u64>    _kv_access;
    StringPool  *_string_pool;
    Array<char*> _errors;
  };

  struct JsonSchemaError
  {
    char *message;
    char *path;
  };
}
