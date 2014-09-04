#pragma once

#include "types.h"

namespace pge
{
  bool load_schemas(const char *schemas_folder, Allocator &a, StringPool &sp);
  void unload_schemas(void);
  
  bool validate_resource (Json &jsn, const ResourceType type, const char *file);
  bool validate_compile_config (Json &jsn);
  bool validate_project_compilers_config(Json &jsn, const char *src);
  bool validate_project_settings_config(Json &jsn, const char *src);
}