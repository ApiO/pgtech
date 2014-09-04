#pragma once

#include <data/shader.h>

#include "resource_manager.h"

namespace pge
{
  namespace shader_resource
  {
    bool has_vertex(const ShaderResource *res);
    bool has_fragment(const ShaderResource *res);

    const char *get_vertex(const ShaderResource *res);
    const char *get_fragment(const ShaderResource *res);

    u32 get_vertex_size(const ShaderResource *res);
    u32 get_fragment_size(const ShaderResource *res);
  }
  namespace shader_resource
  {

    inline bool has_vertex(const ShaderResource *res)
    {
      return res->vertex_shader_size > 0u;
    }

    inline bool has_fragment(const ShaderResource *res)
    {
      return res->fragment_shader_size > 0u;
    }

    inline const char *get_vertex(const ShaderResource *res)
    {
      return ((const char*)res) + res->vertex_shader_offset;
    }

    inline const char *get_fragment(const ShaderResource *res)
    {
      return ((const char*)res) + res->vertex_shader_offset + res->vertex_shader_size;
    }

    inline u32 get_vertex_size(const ShaderResource *res)
    {
      return res->vertex_shader_size;
    }

    inline u32 get_fragment_size(const ShaderResource *res)
    {
      return res->fragment_shader_size;
    }
  }
}