#include <runtime/types.h>
#include <runtime/trace.h>

#include <data/level.h>

#include "compiler_types.h"
#include "compiler.h"

namespace pge
{
  LevelCompiler::LevelCompiler(Allocator &a, StringPool &sp)
    : JsonCompiler(a, RESOURCE_TYPE_LEVEL, sp) {}

  bool LevelCompiler::compile(Work &w)
  {
    if (!load_json(w)) return false;

    LevelResource level;

    u64 units = json::has(jsn, json::root(jsn), "units") ? json::get_id(jsn, json::root(jsn), "units") : 0u;
    u64 sprites = json::has(jsn, json::root(jsn), "sprites") ? json::get_id(jsn, json::root(jsn), "sprites") : 0u;

    // Writes infos
    level.num_units    = units ? json::size(jsn, units) : 0u;
    level.units_offset = sizeof(LevelResource);

    level.num_sprites    = sprites ? json::size(jsn, sprites) : 0u;
    level.sprites_offset = level.units_offset + level.num_units * sizeof(LevelResource::Resource);
      
    fwrite(&level, sizeof(LevelResource), 1, w.data);

    // Writes units 
    if (units){
      u64 node_id;
      LevelResource::Resource unit;

      for (u32 i = 0; i < json::size(jsn, units); i++)
      {
        node_id = json::get_id(jsn, units, i);
        if (!json::has(jsn, node_id, "translation")) {
          LOG("Attribute \"translation\" is missing in \"units > object (%d)\" in file  \"%s\"", i, w.src);
          return false;
        }
        
        unit.name = compiler::create_reference(w, RESOURCE_TYPE_UNIT, json::get_string(jsn, node_id, "name"));
        compiler::read_json_pose(jsn, node_id, unit.pose);

        fwrite(&unit, sizeof(LevelResource::Resource), 1, w.data);
      }
    }

    // Writes sprites 
    if (sprites){
      u64 node_id;
      LevelResource::Resource sprite;

      for (u32 i = 0; i < json::size(jsn, sprites); i++)
      {
        node_id = json::get_id(jsn, sprites, i);
        if (!json::has(jsn, node_id, "translation")) {
          LOG("Attribute \"translation\" is missing in \"sprites > object (%d)\" in file  \"%s\"", i, w.src);
          return false;
        }

        sprite.name = compiler::create_reference(w, RESOURCE_TYPE_UNIT, json::get_string(jsn, node_id, "name"));
        compiler::read_json_pose(jsn, node_id, sprite.pose);

        fwrite(&sprite, sizeof(LevelResource::Resource), 1, w.data);
      }
    }


    return true;
  }
}