#include <string>
#include <stdarg.h>

#include "runtime/json.h"
#include "runtime/assert.h"
#include "runtime/idlut.h"
#include "runtime/hash.h"
#include "runtime/murmur_hash.h"
#include "runtime/string_pool.h"
#include "runtime/string_stream.h"
#include "runtime/temp_allocator.h"
#include "runtime/file_system.h"

#include "libjson/json.h"

namespace
{
  using namespace pge;
  using namespace json;

  const Json::Node DEFAULT_NODE ={ 0 };

  const char *PARSING_ERRORS[] =
  {
    "No error",
    "Out of memory",
    "Bad character",
    "Stack empty",
    "Pop unexpected mode",
    "Nesting limit",
    "Data limit",
    "Comment not allowed by config",
    "Unexpected char",
    "Missing unicode low surrogate",
    "Unexpected unicode low surrogate",
    "Error comma out of structure",
    "Error in a callback"
  };

  inline void push_parsing_error(Json &doc, i32 line, i32 col, const char *format, ...)
  {
    va_list args;
    char    tmp[512];
    u32     offset = 0;
    
    offset += sprintf(tmp + offset, "Line %d, col %d: ", line, col);
    va_start(args, format);
    offset += vsprintf(tmp + offset, format, args);
    va_end(args);

    array::push_back(doc._errors, string_pool::acquire(*doc._string_pool, tmp));
  }

  inline u64 attribute_id(u64 id, const char *attr, u32 attr_len)
  {
    TempAllocator64 ta;
    const u32 key_len = sizeof(u64)+attr_len;
    char *key = (char*)ta.allocate(key_len);

    memcpy(key, &id, sizeof(u64));
    memcpy(key + sizeof(u64), attr, attr_len);

    return murmur_hash_64(key, 0, key_len);
  }

  u64 create_node(Json &doc, u64 parent, JsonType type, const char *name, u32 name_len, u64 value, u32 string_len, i32 line = -1, i32 col = -1)
  {
    u64    id;
    Json::Node n, *np;

    n.prev  = NO_NODE;
    n.next  = NO_NODE;
    n.child = NO_NODE;
    n.type  = type;
    n.name  = name ? string_pool::acquire(*doc._string_pool, name, name_len) : 0;

    if (type == pge::JSON_STRING)
      n.value.string = string_pool::acquire(*doc._string_pool, (char*)value, string_len);
    else
      n.value.raw = value;

    id = idlut::add(doc._nodes, n);
    np = idlut::lookup(doc._nodes, id);
    np->id = id;

    if (n.type == pge::JSON_OBJECT || n.type == pge::JSON_ARRAY)
      np->value.raw = np->id;

    if (name) {
      const u64 attr_id = attribute_id(parent, name, name_len);
      bool valid = !hash::has(doc._kv_access, attr_id);

      if (!valid) {
        if (line >= 0 && col >= 0) {
          char szname[512];
          strncpy(szname, name, name_len);
          szname[name_len] = '\0';
          push_parsing_error(doc, line, col, "The key \"%s\" is already defined", szname);
        }
        return NO_NODE;
      }

      // add the newly created node into the kv hashset
      hash::set(doc._kv_access, attr_id, id);
    }

    if (parent == NO_NODE) return id;

    { // link the new node to the last child of his parent
      Json::Node *p = idlut::lookup(doc._nodes, parent); // parent
      if (p->child == NO_NODE) {
        np->prev = p->id;
        return p->child = id;
      }

      p = idlut::lookup(doc._nodes, p->child);
      while (p->next != NO_NODE)
        p = idlut::lookup(doc._nodes, p->next);
      np->prev = p->id;
      return p->next = id;
    }
  }

  inline u64 get_parent(Json &doc, u64 id)
  {
    XASSERT(idlut::has(doc._nodes, id), "There is no node matching this id : %llu", id);
    Json::Node *n = idlut::lookup(doc._nodes, id);
    if (n->prev == NO_NODE) return NO_NODE;
    n = idlut::lookup(doc._nodes, n->prev);
    while (n->child != id) {
      id = n->id;
      n = idlut::lookup(doc._nodes, n->prev);
    } return n->id;
  }

  inline i32 string_len(const char *text, u32 *pos, u32 *len)
  {
    *len = 0;
    while (text[*pos] != '\0') {
      if (text[*pos] == '\"') return 1;
      ++*pos; ++*len;
    }
    return 0;
  }

  inline i32 primitive_len(const char *text, u32 *pos, u32 *len)
  {
    *len = 0;
    while (text[*pos] != '\0') {
      switch (text[*pos]) {
      case '\t': case '\r': case '\n': case ' ': case  ',': case  ']': case  '}':
        --*pos;
        return 1;
      default: break;
      }
      ++*pos; ++*len;
    }
    return 0;
  }

  void write_children(Json &doc, u64 node, string_stream::Buffer &buf, bool formatted, i32 depth)
  {
    using namespace string_stream;

    u64 nid = node;
    while (nid != NO_NODE) {
      const Json::Node &n = get_node(doc, nid);

      if (nid != node) buf << ',';

      if (formatted) {
        if (n.name) {
          buf << '\n';
          for (int i = 0; i < depth; i++) buf << "  ";
        } else {
          buf << " ";
        }
      }

      if (n.name) { // display the property name
        buf << '"' << n.name << "\":";
        if (formatted) buf << ' ';
      }

      switch (n.type) {
      case pge::JSON_BOOLEAN: buf << (n.value.boolean ? "true" : "false"); break;
      case pge::JSON_NULL:    buf << "null"; break;
      case pge::JSON_INTEGER: buf << n.value.integer; break;
      case pge::JSON_NUMBER:  buf << n.value.number;  break;
      case pge::JSON_STRING:  buf << '"' << n.value.string << '"'; break;
      case pge::JSON_OBJECT:
        buf << '{';
        if (n.child != NO_NODE)
          write_children(doc, n.child, buf, formatted, depth + 1);
        if (formatted) {
          buf << '\n';
          for (int i = 0; i < depth; i++) buf << "  ";
        }
        buf << '}';
        break;
      case pge::JSON_ARRAY:
        buf << '[';
        if (n.child != NO_NODE)
          write_children(doc, n.child, buf, formatted, depth + 1);
        buf << ']';
        break;
      default: break;
      }
      nid = n.next;
    }
  }

  bool check_syntax(Json &doc, const char *str)
  {
    json_parser parser;
    i32 ret, lines, col;
    u32 i, processed;
    json_config config;

    memset(&config, 0, sizeof(json_config));
    config.max_nesting = 0;
    config.max_data = 0;
    config.allow_c_comments = 1;
    config.allow_yaml_comments = 1;

    ret = json_parser_init(&parser, &config, NULL, NULL);
    XASSERT(!ret, "Initializing parser failed (code=%d): %s\n", ret, PARSING_ERRORS[ret]);

    ret   = 0;
    lines = 1;
    col   = 0;
    ret = json_parser_string(&parser, str, strlen(str), &processed);
    for (i = 0; i < processed; i++) {
      if (str[i] == '\n') { col = 0; lines++; } else col++;
    }

    if (ret) {
      push_parsing_error(doc, lines, col, PARSING_ERRORS[ret]);
      return false;
    }

    ret = json_parser_is_done(&parser);
    if (!ret) {
      push_parsing_error(doc, lines, col, "Syntax error");
      return false;
    }
    return true;
  }
}

namespace pge
{
  namespace json
  {
    void get_last_errors(Json &doc, Array<char*> &errors)
    {
      array::copy(errors, doc._errors); array::clear(doc._errors);
    }

    u64 clear(Json &doc)
    {
      // release strings
      IdLookupTable<Json::Node>::Entry *n = idlut::begin(doc._nodes);
      IdLookupTable<Json::Node>::Entry *m = idlut::end(doc._nodes);

      while (n < m) {
        if (n->value.type == pge::JSON_STRING)
          string_pool::release(*doc._string_pool, n->value.value.string);
        if (n->value.name)
          string_pool::release(*doc._string_pool, n->value.name);
        ++n;
      }

      hash::clear(doc._kv_access);
      idlut::clear(doc._nodes);

      for (u32 i = 0; i < array::size(doc._errors); i++)
        string_pool::release(*doc._string_pool, doc._errors[i]);

      array::clear(doc._errors);

      return doc._root = create_node(doc, NO_NODE, pge::JSON_OBJECT, 0, 0, 0, 0);
    }

    void reserve(Json &doc, u32 capacity)
    {
      idlut::reserve(doc._nodes, capacity);
      hash::reserve(doc._kv_access, capacity);
    }

    bool has(const Json &doc, u64 id, const char *attr)
    {
      return hash::get(doc._kv_access, attribute_id(id, attr, strlen(attr)), NO_NODE) != NO_NODE;
    }

    const Json::Node &get_node(const Json &doc, u64 id)
    {
      XASSERT(idlut::has(doc._nodes, id), "There is no node mathing this id : %llu", id);
      return idlut::lookup(doc._nodes, id, DEFAULT_NODE);
    }

    Json::Node &get_node(Json &doc, u64 id)
    {
      XASSERT(idlut::has(doc._nodes, id), "There is no node mathing this id : %llu", id);
      return *idlut::lookup(doc._nodes, id);
    }

    const Json::Node &get_node(const Json &doc, u64 id, const char *attr)
    {
      id = hash::get(doc._kv_access, attribute_id(id, attr, strlen(attr)), NO_NODE);
      XASSERT(id != NO_NODE, "There is no \"%s\" attribute in the node with this id : %llu", attr, id);
      return idlut::lookup(doc._nodes, id, DEFAULT_NODE);
    }

    const Json::Node &get_node(const Json &doc, u64 id, i32 i)
    {
      XASSERT(idlut::has(doc._nodes, id), "There is no node mathing this id : %llu", id);
      id = idlut::lookup(doc._nodes, id, DEFAULT_NODE).child;

      i32 pos = 0;
      while (id != NO_NODE) {
        if (pos == i) break;
        id = idlut::lookup(doc._nodes, id, DEFAULT_NODE).next;
        ++pos;
      }
      XASSERT(pos == i, "The index is out of bounds.");
      return idlut::lookup(doc._nodes, id, DEFAULT_NODE);
    }

    i32 size(const Json &doc, u64 id)
    {
      i32 size = 0;
      XASSERT(idlut::has(doc._nodes, id), "could not find the node is the %llu identifier", id);
      u64 next = idlut::lookup(doc._nodes, id, DEFAULT_NODE).child;

      while (next != NO_NODE) {
        next = idlut::lookup(doc._nodes, next, DEFAULT_NODE).next;
        ++size;
      }
      return size;
    }

    bool parse_from_string(Json &doc, u64 id, const char *str, bool syntax_checking)
    {
      u32 pos;
      i32 name_expected = 1, in_array = 0, root = 1;
      char *name = 0, *val = 0;
      u32 name_len = 0, val_len = 0;
      string_stream::Buffer pbuf(*doc._string_pool->_allocator);

      if (syntax_checking && !check_syntax(doc, str))
        return false;

      for (pos = 0; str[pos] != '\0'; pos++) {
        switch (str[pos]) {
        case '\t': case '\r': case '\n': case ':': case ',': case ' ':
          break;
        case '\"':
          if (name_expected) {
            name = (char*)&str[++pos];
            string_len(str, &pos, &name_len);
            name_expected = 0;
          } else {
            val = (char*)&str[++pos];
            string_len(str, &pos, &val_len);
            if (NO_NODE == create_node(doc, id, pge::JSON_STRING, name, name_len, (u64)val, val_len))
              return false;
            if (!in_array) name_expected = 1;
          } break;
        case '{':
          if (root) { root = 0; break; }
          id = create_node(doc, id, pge::JSON_OBJECT, name, name_len, 0, 0);
          name_expected = 1;
          in_array      = 0;
          break;
        case '[':
          id = create_node(doc, id, pge::JSON_ARRAY, name, name_len, 0, 0);
          name_expected = 0;
          name          = 0;
          name_len      = 0;
          in_array      = 1;
          break;
        case '}': case ']':
          id = get_parent(doc, id);
          if (id == NO_NODE) return true;
          in_array = idlut::lookup(doc._nodes, id)->type == pge::JSON_ARRAY;
          if (in_array) {
            name_expected = 0;
            name          = 0;
            name_len      = 0;
            in_array      = 1;
          } else {
            name_expected = 1;
          }
          break;
        default:
          val = (char*)&str[pos];
          primitive_len(str, &pos, &val_len);
          string_stream::push(pbuf, val, val_len);
          {
            u64 nid = NO_NODE;
            // true? false? null? number? integer
            if (strncmp(string_stream::c_str(pbuf), "true", sizeof("true")) == 0)
              nid = create_node(doc, id, pge::JSON_BOOLEAN, name, name_len, true, 0);
            else if (strncmp(string_stream::c_str(pbuf), "false", sizeof("false")) == 0)
              nid = create_node(doc, id, pge::JSON_BOOLEAN, name, name_len, false, 0);
            else if (strncmp(string_stream::c_str(pbuf), "null", sizeof("null")) == 0)
              nid = create_node(doc, id, pge::JSON_NULL, name, name_len, 0, 0);
            else if (strchr(string_stream::c_str(pbuf), '.') != NULL) {
              union{ u64 asU64; f64 asF64; } u;
              u.asF64 = atof(string_stream::c_str(pbuf));
              nid = create_node(doc, id, pge::JSON_NUMBER, name, name_len, u.asU64, 0);
            } else
              nid = create_node(doc, id, pge::JSON_INTEGER, name, name_len, atoi(string_stream::c_str(pbuf)), 0);

            if (nid == NO_NODE)
              return false;
          }

          array::clear(pbuf);
          if (!in_array) name_expected = 1;
          break;
        }
        if (id == NO_NODE)
          return false;
      }
      return true;
    }

    bool parse_from_file(Json &doc, u64 id, const char *path, bool syntax_checking)
    {
      XASSERT(file_system::file_exists(path), "File not found: \"%s\"", path);

      FILE *stream = fopen(path, "rb");
      Allocator &a = *doc._nodes._data._allocator;
      u32 size;

      string_stream::Buffer buf(a);

      // obtains file size
      fseek(stream, 0, SEEK_END);
      size = ftell(stream);
      rewind(stream);

      ASSERT(size);

      array::resize(buf, size);

      // copy the file into the buffer:
      fread(buf._data, 1, size, stream);
      buf._data[size] = '\0';

      fclose(stream);

      return parse_from_string(doc, id, buf._data, syntax_checking);
    }

    void write(Json &doc, u64 id, Array<char> &buffer, bool formatted)
    {
      using namespace string_stream;
      const Json::Node &n = get_node(doc, id);
      Buffer &buf = buffer;

      switch (n.type) {
      case pge::JSON_BOOLEAN: buf << (n.value.boolean ? "true" : "false"); break;
      case pge::JSON_NULL:    buf << "null"; break;
      case pge::JSON_INTEGER: buf << n.value.integer; break;
      case pge::JSON_NUMBER:  buf << n.value.number;  break;
      case pge::JSON_STRING:  buf << '"' << n.value.string << '"'; break;
      case pge::JSON_OBJECT:
        buf << '{';
        if (n.child != NO_NODE)
          write_children(doc, n.child, buf, formatted, 1);
        if (formatted) buf << '\n';
        buf << '}';
        break;
      case pge::JSON_ARRAY:
        buf << '[';
        if (n.child != NO_NODE)
          write_children(doc, n.child, buf, formatted, 1);
        buf << ']';
        break;
      default: break;
      }
    }

    inline u64 copy_node(Json &doc, u64 parent, const Json &src_doc, const Json::Node &src_node, const char *new_name = 0)
    {
      JsonType src_type  = src_node.type;
      u64      src_child = src_node.child;

      const char *name     = new_name ? new_name : src_node.name;
      const u32   name_len = name ? strlen(name) : 0;

      parent = create_node(
        doc, parent, src_node.type, name, name_len, src_node.value.raw,
        src_node.type == pge::JSON_STRING ? strlen(src_node.value.string) : 0);

      if (src_type != pge::JSON_OBJECT && src_type != pge::JSON_ARRAY) return parent;

      u64 child = src_child;
      while (child != NO_NODE) {
        const Json::Node &n = idlut::lookup(src_doc._nodes, child, DEFAULT_NODE);
        child = n.next;
        copy_node(doc, parent, src_doc, n);
      }

      return parent;
    }

    inline void remove_node(Json &doc, u64 parent, u64 id)
    {
      Json::Node *n = idlut::lookup(doc._nodes, id);
      switch (n->type) {
      case pge::JSON_OBJECT: case pge::JSON_ARRAY:
      {
        u64 child = n->child;
        while (child != NO_NODE) {
          u64 next = idlut::lookup(doc._nodes, child, DEFAULT_NODE).next;
          remove_node(doc, id, child);
          child = next;
        }
      }
        break;
      case pge::JSON_STRING:
        string_pool::release(*doc._string_pool, n->value.string); break;
      default: break;
      }
      if (n->name) {
        hash::remove(doc._kv_access, attribute_id(parent, n->name, strlen(n->name)));
        string_pool::release(*doc._string_pool, n->name);
      }

      if (n->prev != NO_NODE)
        idlut::lookup(doc._nodes, n->prev)->next = n->next;
      if (n->next != NO_NODE)
        idlut::lookup(doc._nodes, n->next)->prev = n->prev;
      idlut::remove(doc._nodes, id);
    }

    inline void replace_node(Json &doc, u64 id, const Json &src_doc, const Json::Node &src_node)
    {
      Json::Node *n = idlut::lookup(doc._nodes, id);
      // delete the old value
      switch (n->type) {
      case pge::JSON_OBJECT: case pge::JSON_ARRAY:
      {
        u64 child = n->child;
        while (child != NO_NODE) {
          u64 next = idlut::lookup(doc._nodes, child, DEFAULT_NODE).next;
          remove_node(doc, id, child);
          child = next;
        }
      }
        break;
      case pge::JSON_STRING:
        string_pool::release(*doc._string_pool, n->value.string); break;
      default: break;
      }

      // copy the new one
      n->type = src_node.type;
      switch (n->type) {
      case pge::JSON_OBJECT: case pge::JSON_ARRAY:
      {
        u64 child = src_node.child;
        while (child != NO_NODE) {
          const Json::Node &cn = idlut::lookup(src_doc._nodes, child, DEFAULT_NODE);
          child = cn.next;
          copy_node(doc, id, src_doc, cn);
        }
      }
        break;
      case pge::JSON_STRING:
        n->value.string = string_pool::acquire(*doc._string_pool, src_node.value.string); break;
      default:
        n->value.raw = src_node.value.raw; break;
      }
    }

    void copy(Json &dest_doc, const Json &src_doc, u64 dest_parent_id, u64 src_id, const char *new_name)
    {
      copy_node(dest_doc, dest_parent_id, src_doc, idlut::lookup(src_doc._nodes, src_id, DEFAULT_NODE), new_name);
    }

    void merge(Json &dest_doc, const Json &src_doc, u64 dest_id, u64 src_id, bool overwrite)
    {
      u64 next = idlut::lookup(src_doc._nodes, src_id, DEFAULT_NODE).child;

      while (next != NO_NODE) {
        const Json::Node &src = idlut::lookup(src_doc._nodes, next, DEFAULT_NODE);
        const u64 dest = hash::get(dest_doc._kv_access, attribute_id(dest_id, src.name, strlen(src.name)), NO_NODE);
        const u64 src_next = src.next;

        if (dest == NO_NODE) {
          // the attribute does not exists in the dest id : copy it from the source id
          copy_node(dest_doc, dest_id, src_doc, src);
        } else {
          // the attribute exists in the dest id
          const Json::Node &tmp = idlut::lookup(dest_doc._nodes, dest, DEFAULT_NODE);
          if (tmp.type == pge::JSON_OBJECT && src.type == pge::JSON_OBJECT) {
            // dest & src attributes are both ids, merge them.
            merge(dest_doc, src_doc, dest, next, overwrite);
          } else if (overwrite) {
            // override the dest value with the src value
            replace_node(dest_doc, dest, src_doc, src);
          }
        }
        next = src_next;
      }
    }
  }

  Json::Json(Allocator &a, StringPool &sp) :
    _nodes(a), _kv_access(a), _string_pool(&sp), _errors(a)
  {
    _root = create_node(*this, json::NO_NODE, pge::JSON_OBJECT, 0, 0, 0, 0);
  }

  Json::Json(const Json &other, u64 root) :
    _nodes(*other._nodes._data._allocator),
    _kv_access(*other._kv_access._data._allocator),
    _string_pool(other._string_pool),
    _errors(other._errors)
  {
    _root = json::copy_node(*this, json::NO_NODE, other,
                            idlut::lookup(other._nodes, root, DEFAULT_NODE));
  }

  Json::~Json()
  {
    json::clear(*this);
  }
}