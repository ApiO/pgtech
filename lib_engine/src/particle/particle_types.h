#pragma once

#include <runtime/collection_types.h>
#include <engine/pge_types.h> // (color & vectors)
#include <data/particle.h>
#include "pose.h"

// http://bitsquid.blogspot.fr/2012_06_01_archive.html
// http://bitsquid.blogspot.fr/2010/02/blob-and-i.html
// http://www.bitsquid.se/files/lua_api.html#particles

namespace pge
{
  struct ParticleCache
  {
    u32 vao, vertices, tex_coords;
    u32 texture;
  };

  struct ParticleEmitter
  {
    bool       active;
    bool       hot;
    u32        sequence;
    f64        accu_spawn;
    f64        accu_despawn;
    Pose       pose;
    u32        num_particles;
    u32        max_particles;
    glm::vec3 *positions;
    glm::vec3 *velocities;
    glm::vec3 *scales;
    glm::vec4 *colors;
    glm::vec4  color_factor;
    glm::vec3  scale_factor;
    const ParticleResource *res;
  };

  struct ParticleSystem
  {
    ParticleSystem(Allocator &a);

    IdLookupTable<ParticleEmitter> emitters;
    Hash<ParticleCache> rendered_emitters;
    u32 poses_texture;
    u32 colors_texture;
  };
}