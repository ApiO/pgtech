#pragma once

#include <stdio.h>

#include <runtime/types.h>
#include <runtime/memory_types.h>
#include <data/types.h>

namespace pge
{
  typedef void *(*ResourceLoad)   (FILE *file, u32 name, u32 size);
  typedef void(*ResourceBringIn)  (void *data, u32 name);
  typedef void(*ResourcePatchUp)  (void *data, u32 name);
  typedef void(*ResourceBringOut) (void *data, u32 name);
  typedef void(*ResourceUnload)   (void *data, u32 name);

  namespace resource_manager
  {
    void *default_load(FILE *file, u32 name, u32 size);
    void default_bring_in(void *data, u32 name);
    void default_patch_up(void *data, u32 name);
    void default_bring_out(void *data, u32 name);
    void default_unload(void *data, u32 name);

    void  init(const char *data_dir, Allocator &a);
    void  register_type(ResourceType     type,
                        ResourceLoad     load,
                        ResourceBringIn  bring_in,
                        ResourcePatchUp  patch_up,
                        ResourceBringOut bring_out,
                        ResourceUnload   unload);

    // If set to ture, the resources will be automatically loaded as they are accessed.
    // The user must call flush_autoloaded_references to flush the autoloaded resource references.
    void  set_autoload (bool enabled);

    // Shutdown the resource manager and unload all resources.
    void  shutdown();

    // Asynchronously loads the memory resident part of the specified resource.
    void  load(ResourceType type, u32 name);

    // Waits for the specified resource to be online.
    void  flush(ResourceType type, u32 name);

    // Calls the patch up callback of the specified resource.
    void  patch_up(ResourceType type, u32 name);

    // Asynchronously unloads the specified resource.
    void  unload(ResourceType type, u32 name);

    // Check if the specified resource has been loaded.
    bool  has_loaded(ResourceType type, u32 name);

    // Returns a pointer to the specified resource data.
    void *get(ResourceType type, u32 name);

    // Opens a stream for the streaming part of the specified resource.
    FILE *open_stream(ResourceType type, u32 name);

    // Returns the name of the specified resource pointer or 0
    u32 name(void *resource);
  }
}
