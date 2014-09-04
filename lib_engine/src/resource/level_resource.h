#include <data/level.h>

#include "resource_manager.h"


namespace pge
{
  namespace level_resource
  {
    u32 num_units    (const LevelResource *res);
    u32 num_spritess (const LevelResource *res);
    const LevelResource::Resource *get_units   (const LevelResource *res);
    const LevelResource::Resource *get_sprites (const LevelResource *res);
  }
  namespace level_resource
  {
    u32 num_units(const LevelResource *res)
    {
      return res->num_units;
    }

    const LevelResource::Resource *get_units(const LevelResource *res)
    {
      return (const LevelResource::Resource*)((u8*)res + res->units_offset);
    }

    u32 num_spritess(const LevelResource *res)
    {
      return res->num_sprites;
    }

    const LevelResource::Resource *get_sprites(const LevelResource *res)
    {
      return (const LevelResource::Resource*)((u8*)res + res->sprites_offset);
    }
  }
}