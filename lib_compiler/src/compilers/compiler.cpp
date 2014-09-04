#include <runtime/temp_allocator.h>
#include <runtime/string_stream.h>
#include <runtime/file_system.h>
#include <runtime/murmur_hash.h>
#include <runtime/trace.h>

#include <schema.h>
#include <linkage_manager.h>
#include <id_string_manager.h>
#include "compiler_types.h"
#include "compiler.h"

namespace
{
  using namespace pge;

  bool has_dependency(const LinkageManager &mng, u64 res_id, u64 dep_id)
  {
    TempAllocator512 ta;
    Array<u64> dependencies(ta);
    linkage_manager::get_dependencies(mng, res_id, dependencies);
    for (u32 i = 0; i < array::size(dependencies); i++) {
      if (dependencies[i] == dep_id)
        return true;
    }
    return false;
  }
}

namespace pge
{
  using namespace string_stream;

  namespace compiler
  {
    bool load_dependency(Work &w, ResourceType type, const char *name, string_stream::Buffer &buf)
    {
      char res_path[MAX_PATH];
      switch (type) {
        case RESOURCE_TYPE_DATA:
          sprintf(res_path, "%s\\%s", w.project->src_dir, name);
          break;
        default:
          sprintf(res_path, "%s\\%s.%s", w.project->src_dir, name, ResourceExtension[type]);
      }

      if (!file_system::file_exists(res_path)) {
        LOG("file not found: \"%s\"", res_path);
        return false;
      }

      if (load_bytes(res_path, buf)) {
        DataId dep_id;
        dep_id.fields.type = type;
        dep_id.fields.name = id_string_manager::create(w.project->id_string_ctx, name);

        if (!has_dependency(w.project->linkage_ctx, w.id.as64, dep_id.as64))
          linkage_manager::add_dependency(w.project->linkage_ctx, w.id.as64, dep_id.as64);

        return true;
      }
      return false;
    }

    bool load_dependency(Work &w, ResourceType type, const char *name, Json &jsn)
    {
      char res_path[MAX_PATH];
      switch (type) {
        case RESOURCE_TYPE_DATA:
          sprintf(res_path, "%s\\%s", w.project->src_dir, name);
          break;
        default:
          sprintf(res_path, "%s\\%s.%s", w.project->src_dir, name, ResourceExtension[type]);
      }

      if (!file_system::file_exists(res_path)) {
        //sets the color to intense red on blue background
        HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        LOG("file not found: \"%s\"", res_path);
        //reverting back to the normal color
        SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        return false;
      }

      if (load_json(w, res_path, type, jsn)) {
        DataId dep_id;
        dep_id.fields.type = type;
        dep_id.fields.name = id_string_manager::create(w.project->id_string_ctx, name);

        if (!has_dependency(w.project->linkage_ctx, w.id.as64, dep_id.as64))
          linkage_manager::add_dependency(w.project->linkage_ctx, w.id.as64, dep_id.as64);

        return true;
      }
      return false;
    }

    u32 create_reference(Work &w, ResourceType type, const char *name)
    {
      DataId ref_id;
      ref_id.fields.name = murmur_hash_32(name);
      ref_id.fields.type = (u32)type;
      linkage_manager::add_reference(w.project->linkage_ctx, w.id.as64, ref_id.as64);
      return ref_id.fields.name;
    }

    u32 create_id_string(Work &w, const char *res_name)
    {
      return id_string_manager::create(w.project->id_string_ctx, res_name);
    }

    bool load_bytes(const char *path, string_stream::Buffer &buf)
    {
      array::clear(buf);

      FILE *stream = fopen(path, "rb");
      if (!stream) {
        LOG("File not found : \"%s\"", path);
        return false;
      }

      // obtain file size:
      fseek(stream, 0, SEEK_END);
      u32 size = ftell(stream);
      rewind(stream);

      array::resize(buf, size);

      // copy the file into the buffer:
      fread(buf._data, 1, size, stream);
      buf._data[size] = '\0';

      // terminate
      fclose(stream);

      return true;
    }

    bool load_json(Work &w, const char *path, ResourceType type, Json &jsn)
    {
      FILE *stream;
      u32   size;

      json::clear(jsn);
      
      // open the resource file
      stream = fopen(path, "rb");
      if (!stream) {
        LOG("File not found : \"%s\"", path);
        return false;
      }

      fseek(stream, 0, SEEK_END);
      size = ftell(stream);
      rewind(stream);

      // load it into the memory
      TempAllocator4096 ta(memory_globals::default_allocator());
      Buffer buf(ta);
      array::resize(buf, size);
      fread(buf._data, 1, size, stream);
      fclose(stream);

      // check and parse its content
      const char *src = string_stream::c_str(buf);
      const char *import  = strstr(src, "@import \"");
      const char *content = strchr(src, '{');

      if (!content && !import) {
        LOG("Empty resource \"%s\"", path);
        return false;
      }

      if (content && !json::parse_from_string(jsn, json::root(jsn), content)) {
        Array<char*> errors(ta);
        Buffer error_string(ta);

        json::get_last_errors(jsn, errors);
        for (u32 i = 0; i < array::size(errors); i++)
          error_string << "\r\n\t" << errors[i];
        LOG("Could not parse the resource \"%s\" :%s", path, c_str(error_string));

        return false;
      }

      if (type == RESOURCE_TYPE_DATA) return true;

      if (!import) {
        return validate_resource(jsn, type, path);
      }

      // get the import path
      const char *import_path = strchr(import, '"') + 1;
      const u32   import_len  = strchr(import_path, '#') - import_path;

      const char *id       = import_path + import_len + 1;
      const u32   id_len   = strchr(id, '"') - id;

      char str_path[MAX_PATH] = "";
      char str_id[MAX_PATH];

      strncat(str_path, import_path, import_len);
      strcat(str_path, ".pgi");

      strncpy(str_id, id, id_len);
      str_id[id_len] = '\0';

      Json tmp(ta, *jsn._string_pool);
      if (!compiler::load_dependency(w, RESOURCE_TYPE_DATA, str_path, tmp)) return false;

      u64 section = json::NO_NODE;
      switch (type) {
        case RESOURCE_TYPE_UNIT:     section = json::get_id(tmp, json::root(tmp), "units");    break;
        case RESOURCE_TYPE_ACTOR:    section = json::get_id(tmp, json::root(tmp), "actors");   break;
        case RESOURCE_TYPE_ANIMSET:  section = json::get_id(tmp, json::root(tmp), "animsets"); break;
        case RESOURCE_TYPE_SPRITE:   section = json::get_id(tmp, json::root(tmp), "sprites");  break;
        default:
          LOG("Unhandled ResourceType enum value %d", type);
          return false;
      }

      const u64 import_id = json::get_id(tmp, section, str_id);
      if (import_id == json::NO_NODE) {
        LOG("The object \"%s\" could not be found into the the import \"%s\".", str_id, str_path);
        return false;
      }
      json::merge(jsn, tmp, json::root(jsn), import_id, false);

      return validate_resource(jsn, type, path);
    }
  }
}