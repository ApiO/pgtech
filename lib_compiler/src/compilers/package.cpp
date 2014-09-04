#include <runtime/types.h>
#include <runtime/trace.h>
#include <runtime/temp_allocator.h>

#include <data/types.h>
#include <data/package.h>

#include <linkage_manager.h>
#include "compiler_types.h"
#include "compiler.h"

namespace
{
  using namespace pge;

  ResourceType type_string_to_enum(const char *type_string)
  {
    for (int i = 1; i < NumResourceExtension; i++) {
      if (strstr(type_string, ResourceTypeNames[i]))
        return (ResourceType)i;
    }
    return RESOURCE_TYPE_DATA;
  }

  bool already_inserted(const Hash<u32> &resources, ResourceType type, u32 name)
  {
    if (!hash::has(resources, (u64)type))
      return false;

    const Hash<u32>::Entry *e = multi_hash::find_first(resources, (u64)type);
    while (e) {
      if (e->value == name)
        return true;
      e = multi_hash::find_next(resources, e);
    }
    return false;
  }

  void add_sprite_references(Work &w, const char *sprite_name, Hash<u32> &resources, Json &tmp_jsn)
  {
    json::clear(tmp_jsn);
    compiler::load_dependency(w, RESOURCE_TYPE_SPRITE, sprite_name, tmp_jsn);

    if (!json::has(tmp_jsn, json::root(tmp_jsn), "frames"))
      return;

    const u64 frames = json::get_id(tmp_jsn, json::root(tmp_jsn), "frames");
    const u32 num_frames = json::size(tmp_jsn, frames);

    u64 frame_id;
    for (u32 i = 0; i < num_frames; i++) {
      frame_id = json::get_id(tmp_jsn, frames, i);
      if (json::has(tmp_jsn, frame_id, "texture")) {
        char sz_texture_name[MAX_PATH];
        const char* texture_name = json::get_string(tmp_jsn, frame_id, "texture");
        const u32 texture_len = strcspn(texture_name, "#[");
        XASSERT(texture_len < MAX_PATH, "Texture name too long.");
        strncpy(sz_texture_name, texture_name, texture_len);
        sz_texture_name[texture_len] = '\0';

        multi_hash::insert(resources,
                           (u64)RESOURCE_TYPE_TEXTURE,
                           compiler::create_reference(w, RESOURCE_TYPE_TEXTURE, sz_texture_name));
      }
    }
  }


  void add_actor_references(Work &w, const char *actor_name, Hash<u32> &resources, Json &tmp_jsn)
  {
    json::clear(tmp_jsn);
    compiler::load_dependency(w, RESOURCE_TYPE_ACTOR, actor_name, tmp_jsn);

    const u64 shapes     = json::get_id(tmp_jsn, json::root(tmp_jsn), "shapes");
    const u32 num_shapes = json::size(tmp_jsn, shapes);

    for (u32 i = 0; i < num_shapes; i++) {
      multi_hash::insert(resources,
                         (u64)RESOURCE_TYPE_SHAPE,
                         compiler::create_reference(w, RESOURCE_TYPE_SHAPE,
                         json::get_string(tmp_jsn, json::get_id(tmp_jsn, shapes, i), "shape")));
    }
  }

  void add_unit_references(Work &w, const char *unit_name, Hash<u32> &resources, Json &tmp_jsn)
  {
    json::clear(tmp_jsn);

    compiler::load_dependency(w, RESOURCE_TYPE_UNIT, unit_name, tmp_jsn);

    // add animation set references
    if (json::has(tmp_jsn, json::root(tmp_jsn), "animations")) {
      multi_hash::insert(resources, (u64)RESOURCE_TYPE_ANIMSET,
                         compiler::create_id_string(
                         w, json::get_string(tmp_jsn, json::get_id(tmp_jsn, json::root(tmp_jsn), "animations"), "set")));
    }

    // add physics references
    if (json::has(tmp_jsn, json::root(tmp_jsn), "actors")) {
      Json jsn_actor(*tmp_jsn._nodes._data._allocator, *tmp_jsn._string_pool);
      u64  next = json::get_node(tmp_jsn, json::root(tmp_jsn), "actors").child;
      while (next != json::NO_NODE) {
        const Json::Node &node = json::get_node(tmp_jsn, next);
        const char *actor_name = json::get_string(tmp_jsn, node.id, "actor");
        if (!already_inserted(resources, RESOURCE_TYPE_ACTOR, compiler::create_id_string(w, actor_name))) {
          multi_hash::insert(resources, (u64)RESOURCE_TYPE_ACTOR, compiler::create_id_string(w, actor_name));
          add_actor_references(w, actor_name, resources, jsn_actor);
        }
        next = node.next;
      }
    }

    // add sprites references
    if (json::has(tmp_jsn, json::root(tmp_jsn), "sprites")) {
      Json jsn_sprite(*tmp_jsn._nodes._data._allocator, *tmp_jsn._string_pool);

      u64  next = json::get_node(tmp_jsn, json::root(tmp_jsn), "sprites").child;
      while (next != json::NO_NODE) {
        const Json::Node &node = json::get_node(tmp_jsn, next);
        const char *sprite_name = json::get_string(tmp_jsn, node.id, "template");
        if (!already_inserted(resources, RESOURCE_TYPE_SPRITE, compiler::create_id_string(w, sprite_name))) {
          multi_hash::insert(resources, (u64)RESOURCE_TYPE_SPRITE, compiler::create_id_string(w, sprite_name));
          add_sprite_references(w, sprite_name, resources, jsn_sprite);
        }
        next = node.next;
      }
    }
  }

  void add_level_references(Work &w, const char *level_name, Hash<u32> &resources, Json &tmp_jsn)
  {
    json::clear(tmp_jsn);
    Json dep_jsn(*tmp_jsn._nodes._data._allocator, *tmp_jsn._string_pool);
    compiler::load_dependency(w, RESOURCE_TYPE_LEVEL, level_name, tmp_jsn);

    // add unit references
    if (json::has(tmp_jsn, json::root(tmp_jsn), "units")) {
      u64  next = json::get_node(tmp_jsn, json::root(tmp_jsn), "units").child;
      while (next != json::NO_NODE) {
        const Json::Node &node = json::get_node(tmp_jsn, next);
        const char *unit_name = json::get_string(tmp_jsn, node.id, "name");
        if (!already_inserted(resources, RESOURCE_TYPE_UNIT, compiler::create_id_string(w, unit_name))) {
          multi_hash::insert(resources, (u64)RESOURCE_TYPE_UNIT, compiler::create_id_string(w, unit_name));
          add_unit_references(w, unit_name, resources, dep_jsn);
        }
        next = node.next;
      }
    }

    // add sprite references
    if (json::has(tmp_jsn, json::root(tmp_jsn), "sprites")) {
      u64  next = json::get_node(tmp_jsn, json::root(tmp_jsn), "sprites").child;
      while (next != json::NO_NODE) {
        const Json::Node &node = json::get_node(tmp_jsn, next);
        const char *sprite_name = json::get_string(tmp_jsn, node.id, "name");
        if (!already_inserted(resources, RESOURCE_TYPE_SPRITE, compiler::create_id_string(w, sprite_name))) {
          multi_hash::insert(resources, (u64)RESOURCE_TYPE_SPRITE, compiler::create_id_string(w, sprite_name));
          add_sprite_references(w, sprite_name, resources, dep_jsn);
        }
        next = node.next;
      }
    }
  }

  bool add_references(Work &w, ResourceType type, const char *name, Hash<u32> &resources, Json &tmp_jsn)
  {
    switch (type) {
    case RESOURCE_TYPE_UNIT:
      add_unit_references(w, name, resources, tmp_jsn);
      break;
    case RESOURCE_TYPE_LEVEL:
      add_level_references(w, name, resources, tmp_jsn);
      break;
    case RESOURCE_TYPE_SPRITE:
      add_sprite_references(w, name, resources, tmp_jsn);
      break;
    default:
      compiler::create_reference(w, type, name);
      break;
    }
    return true;
  }
}

namespace pge
{
  PackageCompiler::PackageCompiler(Allocator &a, StringPool &sp)
    : JsonCompiler(a, RESOURCE_TYPE_PACKAGE, sp) {}

  bool PackageCompiler::compile(Work &w)
  {
    if (!load_json(w)) return false;
    Hash<u32> resources(*a);
    Json tmp_jsn(*a, *jsn._string_pool);

    // parse data to extract resources and its references
    u64 next = json::get_node(jsn, json::root(jsn)).child;
    while (next != json::NO_NODE) {
      const Json::Node node = json::get_node(jsn, next);
      const u32 num_resources = json::size(jsn, node.id);
      const ResourceType type = type_string_to_enum(node.name);

      for (u32 i = 0; i < num_resources; i++) {
        const char *name = json::get_string(jsn, node.id, i);
        multi_hash::insert(resources, (u64)type, compiler::create_id_string(w, name));
        if (!add_references(w, type, name, resources, tmp_jsn)) return false;
      }
      next = node.next;
    }

    {
      Array<PackageResource::Type> types(*a);
      Array<u32> all_resources(*a);

      // for each types
      for (u32 i = 1; i <= NumResourceExtension; i++) {
        if (!hash::has(resources, (u64)i))
          continue;

        PackageResource::Type type;
        type.name = i;
        type._resource_offset = 0;

        {
          TempAllocator1024 ta(*a);
          Array<u32> type_resources(ta);

          // get the extracted resources
          multi_hash::get(resources, (u64)i, type_resources);

          // remove duplicate resources
          for (u32 j = 0; j < array::size(type_resources); j++) {
            for (u32 k = j + 1; k < array::size(type_resources); k++) {
              while (type_resources[j] == type_resources[k] && k < array::size(type_resources))
                type_resources[k] = array::pop_back(type_resources);
            }
          }

          // copy in global resources
          for (u32 j = 0; j < array::size(type_resources); j++)
            array::push_back(all_resources, type_resources[j]);

          type.num_resources = array::size(type_resources);
        }
        // store the type
        array::push_back(types, type);
      }

      // write the types
      const u32 num_types = array::size(types);
      fwrite(&num_types, sizeof(u32), 1, w.data);

      const u32 resource_start = sizeof(PackageResource)+(num_types * sizeof(PackageResource::Type));
      u32 offset = 0;
      for (u32 i = 0; i < num_types; i++) {
        types[i]._resource_offset = resource_start + offset;
        offset += types[i].num_resources * sizeof(u32);
        fwrite(&types[i], sizeof(PackageResource::Type), 1, w.data);
      }

      // write the resources
      for (u16 i = 0; i < (u16)array::size(all_resources); i++) {
        fwrite(&all_resources[i], sizeof(u32), 1, w.data);
      }
    }

    return true;
  }
}