#pragma once

#include <runtime/types.h>
#include <data/package.h>

namespace pge
{
  namespace package_resource
  {
    u32 num_types(const PackageResource *package);

    const PackageResource::Type *types(const PackageResource *package);
    const u32 *type_resources(const PackageResource *package, const PackageResource::Type *type);
  }

  namespace package_resource
  {
    inline u32 num_types(const PackageResource *package)
    {
      return package->_num_types;
    }

    inline const PackageResource::Type *types(const PackageResource *package)
    {
      return (PackageResource::Type*)((char*)package + sizeof(PackageResource));
    }

    inline const u32 *type_resources(const PackageResource *package, const PackageResource::Type *type)
    {
      return (u32*)((char*)package + type->_resource_offset);
    }
  }
}