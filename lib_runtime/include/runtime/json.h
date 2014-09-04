#pragma once

#include "runtime/json_types.h"
#include "runtime/assert.h"

namespace pge
{
  namespace json
  {
    const u64 NO_NODE = 0xFFFFFFFFu;

    /// Parses memory block and sotre the result in the node of with the specified id.
    /// If the parse did not ended successfully, errors can be retrieved with "get_last_errors".
    bool parse_from_string(Json &doc, u64 id, const char *strn, bool syntax_checking = true);

    /// Parses a json file and sotre the result in the node of with the specified id.
    /// If the parse did not ended successfully, errors can be retrieved with "get_last_errors".
    bool parse_from_file(Json &doc, u64 id, const char *path, bool syntax_checking = true);

    /// Writes the json into the specified buffer buffer
    void write(Json &doc, u64 id, Array<char> &buf, bool formatted = false);

    /// Validates a json input withe the specified schema.
    /// If the validation did not passed, errors can be retrieved with "get_last_errors".
    bool validate(Json &data_doc, const Json &schema_doc, u64 data_node = NO_NODE, u64 schema_node = NO_NODE);

    /// Returns the last errors produced by parsing or validation.
    /// Do not free the strings: Thoose are atomatically freed when the document is destroyed.
    void get_last_errors(Json &doc, Array<char*> &errors);

    /// Copies the src_id node of the src_doc document under the dest_parent_id node of dest_doc document.
    /// If new_name is specified, it will be used instead of the original node name.
    void copy(Json &dest_doc, const Json &src_doc, u64 dest_parent_id, u64 src_id, const char *new_name = 0);

    /// Merges the src_id node of the src_doc document into the specified dest_id node of dest_doc document.
    /// Attributes that already extist in the destination object will be overriden if _override is set to true.
    void merge(Json &dest_doc, const Json &src_doc, u64 dest_id, u64 src_id, bool overwrite = true);

    /// Returns the root of the specified json document.
    u64  root(const Json &doc);

    /// Returns the size of the specified node id.
    i32  size(const Json &doc, u64 id);

    /// Retruns the type of the value stored for the specified node id.
    JsonType get_type(const Json &doc, u64 id);

    /// Reserves enough memory for the 'capacity' ammount of nodes.
    void reserve(Json &doc, u32 capacity);

    /// Removes all nodes and returns the root.
    u64  clear(Json &doc);

    // ---------------------------------------------------------------
    // Object reading
    // ---------------------------------------------------------------

    /// Returns true if the attribute does exist in the json object.
    bool has(const Json &doc, u64 id, const char *attr);

    /// Returns the size of the specified attribute.
    i32 size(const Json &doc, u64 id, const char *attr);

    /// Returns the identifier of the specified attribute. 
    u64 get_id(const Json &doc, u64 id, const char *attr);

    /// Returns the type of the specified attribute.
    JsonType get_type(const Json &doc, u64 id, const char *attr);

    /// Returns the value stored for the specified attribute.
    bool get_bool(const Json &doc, u64 id, const char *attr);
    i32  get_integer(const Json &doc, u64 id, const char *attr);
    f64  get_number(const Json &doc, u64 id, const char *attr);
    const char *get_string(const Json &doc, u64 id, const char *attr);

    /// Returns the value stored for the specified attribute,
    /// or _default if the attribute does not exists
    bool get_bool(const Json &doc, u64 id, const char *attr, const bool &_default);
    i32  get_integer(const Json &doc, u64 id, const char *attr, const i32  &_default);
    f64  get_number(const Json &doc, u64 id, const char *attr, const f64  &_default);
    const char *get_string(const Json &doc, u64 id, const char *attr, const char *_default);

    // ---------------------------------------------------------------
    // Array reading
    // ---------------------------------------------------------------

    /// Returns the value size for the specified array index, or 0 if the index.
    /// is out of the array bounds.
    i32 size(const Json &doc, u64 id, i32 i);

    /// Returns the identifier of the specified node at i.
    u64 get_id(const Json &doc, u64 id, i32 i);

    /// Returns the type of the value stored for the specified node at i.
    JsonType get_type(const Json &doc, u64 id, i32 i);

    /// Returns the value stored for the specified node at i.
    bool get_bool(const Json &doc, u64 id, i32 i);
    i32  get_integer(const Json &doc, u64 id, i32 i);
    f64  get_number(const Json &doc, u64 id, i32 i);
    const char *get_string(const Json &doc, u64 id, i32 i);

    // ---------------------------------------------------------------
    // Low level reading
    // ---------------------------------------------------------------

    Json::Node &get_node(Json &doc, u64 id); // mowche (utilisé dans le compilo animset.cpp)
    const Json::Node &get_node(const Json &doc, u64 id);
    const Json::Node &get_node(const Json &doc, u64 id, const char *attr);
    const Json::Node &get_node(const Json &doc, u64 id, i32 i);
  }

  // ---------------------------------------------------------------
  // Inline function implementations
  // ---------------------------------------------------------------

  namespace json
  {
    inline u64 root(const Json &doc) { return doc._root; }

    inline i32 size(const Json &doc, u64 id, const char *attr)
    {
      return size(doc, get_node(doc, id, attr).id);
    }

    inline i32 size(const Json &doc, u64 id, i32 i)
    {
      return size(doc, get_node(doc, id, i).id);
    }

    inline JsonType get_type(const Json &doc, u64 id)
    {
      return get_node(doc, id).type;
    }

    // ---------------------------------------------------------------
    // Object reading inline implementation
    // ---------------------------------------------------------------

    inline u64 get_id(const Json &doc, u64 id, const char *attr)
    {
      return get_node(doc, id, attr).id;
    }

    inline JsonType get_type(const Json &doc, u64 id, const char *attr)
    {
      return get_node(doc, id, attr).type;
    }

    inline bool get_bool(const Json &doc, u64 id, const char *attr)
    {
      const Json::Node &n = get_node(doc, id, attr);
      XASSERT(n.type == JSON_BOOLEAN || n.type == JSON_INTEGER,
              "A boolean can only be returned from boolean or integer node.");

      if (n.type == JSON_BOOLEAN)
        return n.value.boolean;
      else
        return n.value.integer != 0;
    }

    inline i32 get_integer(const Json &doc, u64 id, const char *attr)
    {
      const Json::Node &n = get_node(doc, id, attr);
      XASSERT(n.type == JSON_INTEGER, "the value is not an integer");

      return n.value.integer;
    }

    inline f64 get_number(const Json &doc, u64 id, const char *attr)
    {
      const Json::Node &n = get_node(doc, id, attr);
      XASSERT(n.type == JSON_NUMBER || n.type == JSON_INTEGER,
              "A number can only be returned from a number or integer node.");

      if (n.type == JSON_NUMBER)
        return n.value.number;
      else
        return n.value.integer;
    }

    inline const char *get_string(const Json &doc, u64 id, const char *attr)
    {
      const Json::Node &n = get_node(doc, id, attr);
      XASSERT(n.type == JSON_STRING, "the value is not a string");

      return n.value.string;
    }

    inline bool get_bool(const Json &doc, u64 id, const char *attr, const bool &_default)
    {
      if (!has(doc, id, attr)) return _default;
      return get_bool(doc, id, attr);
    }

    inline i32 get_integer(const Json &doc, u64 id, const char *attr, const i32 &_default)
    {
      if (!has(doc, id, attr)) return _default;
      return get_integer(doc, id, attr);
    }

    inline f64 get_number(const Json &doc, u64 id, const char *attr, const f64 &_default)
    {
      if (!has(doc, id, attr)) return _default;
      return get_number(doc, id, attr);
    }

    inline const char *get_string(const Json &doc, u64 id, const char *attr, const char *_default)
    {
      if (!has(doc, id, attr)) return _default;
      return get_string(doc, id, attr);
    }

    // ---------------------------------------------------------------
    // Array reading inline implementation
    // ---------------------------------------------------------------

    inline u64 get_id(const Json &doc, u64 id, i32 i)
    {
      return get_node(doc, id, i).id;
    }

    inline JsonType get_type(const Json &doc, u64 id, i32 i)
    {
      return get_node(doc, id, i).type;
    }

    inline bool get_bool(const Json &doc, u64 id, i32 i)
    {
      const Json::Node &n = get_node(doc, id, i);
      XASSERT(n.type == JSON_BOOLEAN || n.type == JSON_INTEGER,
              "A boolean can only be returned from boolean or integer node.");

      if (n.type == JSON_BOOLEAN)
        return n.value.boolean;
      else
        return n.value.integer != 0;
    }

    inline i32 get_integer(const Json &doc, u64 id, i32 i)
    {
      const Json::Node &n = get_node(doc, id, i);
      XASSERT(n.type == JSON_INTEGER, "the value is not an integer");

      return n.value.integer;
    }

    inline f64 get_number(const Json &doc, u64 id, i32 i)
    {
      const Json::Node &n = get_node(doc, id, i);
      XASSERT(n.type == JSON_NUMBER || n.type == JSON_INTEGER,
              "A number can only be returned from a number or integer node.");

      if (n.type == JSON_NUMBER)
        return n.value.number;
      else
        return n.value.integer;
    }

    inline const char *get_string(const Json &doc, u64 id, i32 i)
    {
      const Json::Node &n = get_node(doc, id, i);
      XASSERT(n.type == JSON_STRING, "the value is not a string");

      return n.value.string;
    }
  }
}