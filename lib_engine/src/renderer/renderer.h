#pragma once

#include <glm/glm.hpp>
#include <engine/pge_types.h>

#include <runtime/types.h>
#include <runtime/memory_types.h>
#include <text/font_resource.h>

#include "renderer_types.h"

namespace pge
{
  const u32 VERTICE_SIZE  = sizeof(glm::vec3);
  const u32 TEXCOORD_SIZE = sizeof(glm::vec2);
  const u32 COLOR_SIZE    = sizeof(RGBA);
  const u32 INDICE_SIZE   = sizeof(u16);

  const u32 SPRITE_VERTICES_SIZE = 4 * VERTICE_SIZE;
  const u32 SPRITE_TEXCOORD_SIZE = 4 * TEXCOORD_SIZE;
  const u32 SPRITE_COLORS_SIZE   = 4 * COLOR_SIZE;
  const u32 SPRITE_INDICES_SIZE  = 6 * INDICE_SIZE;

  namespace renderer
  {
    // Initializes system
    void init(u32 sprite_program, u32 primitive_program, u32 particle_program, 
              RenderInit render_init, RenderBegin render_begin, RenderEnd render_end, RenderShutdown render_shutdown, Allocator &a);
    
    // render frame
    void render(void);

    // Cleanups system allocations
    void shutdown(void);

    void swap_buffers(void);

    // Returns data pointor of specified graphic item
    const u8 *get_data(const Graphic *graphic);

    void start_batch(u32 screen_x, u32 screen_y, u32 width, u32 height, const glm::mat4 &m);

    // Returns pointor of allocation in front graphic's buffer
    u8 *create_graphic(GraphicType type, u32 size, SortKey sort_key);

    // Returns the internal index of the texture resource
    u32 get_texture(u32 texture_name, u32 region_name);

    // Returns the internal index of the font resource
    u32 get_font(const FontResource *font, u32 page);

    // Returns the id of the program used to render the specified graphic type
    u32 get_program_id(GraphicType type);
  }
}