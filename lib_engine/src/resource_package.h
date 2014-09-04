#pragma once

#include <resource/resource_manager.h>
#include <resource/package_resource.h>

namespace pge
{
  namespace resource_package
  {
    inline void load(u64 package)
    {
      resource_manager::flush(RESOURCE_TYPE_PACKAGE, (u32)package);
      const PackageResource *pr = (PackageResource*)resource_manager::get(RESOURCE_TYPE_PACKAGE, (u32)package);
      const PackageResource::Type *types = package_resource::types(pr);

      for (u32 i = 0; i < package_resource::num_types(pr); i++) {
        const u32 *type_resources = package_resource::type_resources(pr, &types[i]);
        for (u32 j = 0; j < types[i].num_resources; j++) {
          resource_manager::load((ResourceType)types[i].name, type_resources[j]);
        }
      }
    }

    inline bool has_loaded(u64 package)
    {
      if (!resource_manager::has_loaded(RESOURCE_TYPE_PACKAGE, (u32)package))
        return false;

      const PackageResource *pr = (PackageResource*)resource_manager::get(RESOURCE_TYPE_PACKAGE, (u32)package);
      const PackageResource::Type *types = package_resource::types(pr);

      for (u32 i = 0; i < package_resource::num_types(pr); i++) {
        const u32 *type_resources = package_resource::type_resources(pr, &types[i]);
        for (u32 j = 0; j < types[i].num_resources; j++) {
          if (!resource_manager::has_loaded((ResourceType)types[i].name, type_resources[j]))
            return false;
        }
      }
      return true;
    }

    inline void flush(u64 package)
    {
      const PackageResource *pr = (PackageResource*)resource_manager::get(RESOURCE_TYPE_PACKAGE, (u32)package);
      const PackageResource::Type *types = package_resource::types(pr);

      bool complete = false;
      while (!complete) {
        complete = true;
        for (u32 i = 0; i < package_resource::num_types(pr); i++) {
          const u32 *type_resources = package_resource::type_resources(pr, &types[i]);
          for (u32 j = 0; j < types[i].num_resources; j++) {
            if (resource_manager::has_loaded((ResourceType)types[i].name, type_resources[j]))
              resource_manager::flush((ResourceType)types[i].name, type_resources[j]);
            else
              complete = false;
          }
        }
      }

      for (u32 i = 0; i < package_resource::num_types(pr); i++) {
        const u32 *type_resources = package_resource::type_resources(pr, &types[i]);
        for (u32 j = 0; j < types[i].num_resources; j++)
          resource_manager::patch_up((ResourceType)types[i].name, type_resources[j]);
      }
    }

    inline void unload(u64 package)
    {
      const PackageResource *pr = (PackageResource*)resource_manager::get(RESOURCE_TYPE_PACKAGE, (u32)package);
      const PackageResource::Type *types = package_resource::types(pr); // resource types of the package
      const u32 *type_resources;                                        // resources of the type
      Application::Resource r;

      for (u32 i = 0; i < package_resource::num_types(pr); i++) {
        type_resources = package_resource::type_resources(pr, &types[i]);
        for (u32 j = 0; j < types[i].num_resources; j++) {
          r.type = (ResourceType)types[i].name;
          r.name = type_resources[j];
          array::push_back(app->resource_unload_queue, r);
        }
      }
    }
  }
}