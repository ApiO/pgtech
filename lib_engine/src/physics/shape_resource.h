#pragma once

#include <data/shape.h>

namespace pge
{
  namespace shape_resource
  {
    void  register_type(void);
    void *load(FILE *file, u32 name, u32 size);
  }

  namespace shape_resource
  {
    inline void register_type(void)
    {
      resource_manager::register_type(RESOURCE_TYPE_SHAPE,
                                      &shape_resource::load,
                                      &resource_manager::default_bring_in,
                                      &resource_manager::default_patch_up,
                                      &resource_manager::default_bring_out,
                                      &resource_manager::default_unload);
    }

    inline void *load(FILE *file, u32 name, u32 size)
    {
      (void)size;
      (void)name;
      ShapeResource sr;
      ShapeData *sd;

      fread(&sr, sizeof(ShapeResource), 1, file);
      sd = (ShapeData*)memory_globals::default_allocator().allocate(sizeof(ShapeData) + sr._num_components * sizeof(f32));
      sd->num_components = sr._num_components;
      sd->type = sr._type;
      sd->components = (f32*)(sd+1);
      fread(sd->components, sizeof(f32), sd->num_components, file);

      return sd;
    }
  }
}