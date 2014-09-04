#pragma once

#include <runtime/types.h>

namespace pge
{
  struct ShaderResource
  {
    u32 vertex_shader_offset;
    u32 vertex_shader_size;
    u32 fragment_shader_size;
  };
}