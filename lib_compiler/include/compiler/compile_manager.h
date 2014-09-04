#pragma once

#include "runtime/types.h"
#include "runtime/memory_types.h"

namespace pge
{
  namespace compile_manager
  {
    bool init(const char *config_folder, const char *schemas_folder, Allocator &a);
    void start_watching (u64 id);
    void stop_watching  (u64 id);
    u64 add (const char *src, const char *data);
    bool project_loaded (u64 id);
    void remove  (u64 id);
    bool build   (u64 id);
    bool rebuild (u64 id);
    void bundle  (u64 id);
    void update   (void);
    void shutdown (void);
  }
}