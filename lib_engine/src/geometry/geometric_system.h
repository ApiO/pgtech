#pragma once

#include <engine/pge_types.h>
#include <renderer/renderer_types.h>
#include "geometric_types.h"

namespace pge
{
  namespace geometric_system
  {
    u64 create_line    (GeometricSystem &s, const glm::vec3 &a, const glm::vec3 &b, const Color &color);
    u64 create_chain   (GeometricSystem &s, const glm::vec3 *vertices, u32 num_vertices, const Color &color);
    u64 create_polygon (GeometricSystem &s, const glm::vec3 *vertices, u32 num_vertices, const Color &color);
    u64 create_box     (GeometricSystem &s, f32 width, f32 height, const Color *colors, bool surface);
    u64 create_circle  (GeometricSystem &s, const glm::vec3 &center, f32 r, const Color &color, bool surface);
    
    void gather_line    (const glm::vec3 &a, const glm::vec3 &b, const Color &color);
    void gather_chain   (const glm::vec3 *vertices, u32 num_vertices, const Color &color);
    void gather_polygon (const glm::vec3 *vertices, u32 num_vertices, const Color &color);
    void gather_box     (f32 width, f32 height, const Color *colors, u32 num_colors, bool surface);
    void gather_circle  (const glm::vec3 &center, f32 r, const Color &color, bool surface);

    Pose &get_pose (GeometricSystem &s, u64 geometry);

    void update  (GeometricSystem &s);
    void gather  (const GeometricSystem &s);
    void draw    (const Graphic *graphics, u32 num_items, RenderBuffer &render_buffer);
    void destroy (GeometricSystem &s, u64 geometry);
  }
}