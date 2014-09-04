#pragma once

#include <runtime/assert.h>
#include <data/texture.h>

namespace pge
{
  namespace texture_resource
  {
    const TextureResource::Region *region(const TextureResource *texture, u32 name, bool *empty);
    const TextureResource::Page   *page(const TextureResource *texture, const TextureResource::Region *region);
    const void                    *page_data(const TextureResource *texture, const TextureResource::Page *page, u32 *size);
  }

  namespace texture_resource_internal
  {
    inline TextureResource::Region *regions(const TextureResource *texture)
    {
      return (TextureResource::Region*)((u8*)texture + sizeof(TextureResource));
    }

    inline u32 *empty_regions(const TextureResource *texture)
    {
      return (u32*)(texture_resource_internal::regions(texture) + texture->num_regions);
    }

    inline TextureResource::Page *pages(const TextureResource *texture)
    {
      return (TextureResource::Page*)(empty_regions(texture) + texture->num_empty_regions);
    }
  }

  namespace texture_resource
  {
    inline const TextureResource::Region *region(const TextureResource *texture, u32 name)
    {
      TextureResource::Region *regions = texture_resource_internal::regions(texture);
      for (u16 i = 0; i < texture->num_regions; i++) {
        if (regions[i].name == name) return &regions[i];
      }

      u32 *empty_regions = texture_resource_internal::empty_regions(texture);
      for (u16 i = 0; i < texture->num_empty_regions; i++) {
        if (empty_regions[i] == name) return 0;
      }

      XERROR("could not find the region %u", name);
      return 0;
    }

    inline const TextureResource::Page *page(const TextureResource *texture, const TextureResource::Region *region)
    {
      TextureResource::Page *pages = texture_resource_internal::pages(texture);
      return &pages[region->page];
    }

    inline const void *page_data(const TextureResource *texture, const TextureResource::Page *page)
    {
      return (u8*)texture + page->data_offset;
    }
  }
}