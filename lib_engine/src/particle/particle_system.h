#pragma once

#include "particle_types.h"
#include <data/particle.h>
#include <renderer/renderer_types.h>

namespace pge
{
  namespace particle_system
  {
    void  init     (ParticleSystem &system);
    void  update   (ParticleSystem &system, f64 dt);
    void  shutdown (ParticleSystem &system);
   
    u64   create   (ParticleSystem &system, const ParticleResource *res);
    void  start    (ParticleSystem &system, u64 emitter);
    void  stop     (ParticleSystem &system, u64 emitter);
    void  destroy  (ParticleSystem &system, u64 emitter);

    Pose &get_pose (ParticleSystem &system, u64 emitter);

    void gather    (ParticleSystem &system);
    void draw      (const Graphic *graphics, u32 num_items, RenderBuffer &render_buffer);
  }
}