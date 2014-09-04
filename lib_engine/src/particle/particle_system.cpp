#include "particle_system.h"
#include <runtime/idlut.h>
#include <glm/gtx/random.hpp>
#include <renderer/renderer.h>
#include <resource/resource_manager.h>
#include <resource/texture_resource.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <utils/ogl_debug.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#define PARTICLE_SIZE ((sizeof(glm::vec3) * 3) + sizeof(glm::vec4))
#define DATA_SIZE     (sizeof(glm::mat4) + sizeof(glm::vec4))
#define PI 3.14159265359f

namespace pge
{
  void inline update_velocity(glm::vec3 &velocity, const f32 *gravity, f32 dt)
  {
    velocity.x += dt * gravity[0];
    velocity.y += dt * gravity[1] * -1;
    velocity.z += dt * gravity[2];
  }

  struct ParticleData
  {
    ParticleSystem *sys;
    u64        id;
    u32        num_particles;
    i32        tex_size[2];
    u32        tex_name;
    u32        tex_region;
  };

  namespace particle_data
  {
    inline glm::mat4 *get_poses(const ParticleData &data)
    {
      return (glm::mat4*)(&data + 1);
    }

    inline glm::vec4 *get_colors(const ParticleData &data)
    {
      return (glm::vec4*)(get_poses(data) + (data.tex_size[0] * data.tex_size[1]));
    }
  }

  const u16 INDICES[] ={ 0u, 1u, 2u, 2u, 3u, 0u };
}

namespace pge
{
  namespace particle_system
  {
    void init(ParticleSystem &system) {
      system.poses_texture = 0;
      system.colors_texture = 0;
    }

    void update(ParticleSystem &system, f64 dt)
    {
      const f32 sdt = dt;
      u32 j;
      IdLookupTable<ParticleEmitter>::Entry *e, *end = idlut::end(system.emitters);

      for (e = idlut::begin(system.emitters); e < end; e++) {
        ParticleEmitter &pe = e->value;
        if (!pe.active)
          continue;

        pose::update(pe.pose);
        pe.accu_spawn += dt;
        pe.accu_despawn += dt;

        // despawn particles
        if (!pe.hot) {
          while (pe.accu_despawn >= pe.res->lifespan && pe.num_particles > 0) {
            --pe.num_particles;
            ++pe.sequence;
            pe.accu_despawn -= pe.res->lifespan;
            pe.hot = true;
            if (pe.sequence == pe.max_particles)
              pe.sequence = 0;
           }
        } else {
          while (pe.accu_despawn >= 1/pe.res->rate && pe.num_particles > 0) {
            --pe.num_particles;
            ++pe.sequence;
            pe.accu_despawn -= 1/pe.res->rate;
            if (pe.sequence == pe.max_particles)
              pe.sequence = 0;
           }
        }

        // update particles
        for (u32 i = 0; i < pe.num_particles; i++) {
          j = (pe.sequence + i) % pe.max_particles;
          update_velocity(pe.velocities[j], pe.res->gravity, sdt);
          pe.positions[j] += pe.velocities[j] * sdt;
          pe.scales   [j] += pe.scale_factor  * sdt;
          pe.colors   [j] += pe.color_factor  * sdt;
        }

        // spawn new particles
        while (pe.accu_spawn >= (1/pe.res->rate) && pe.num_particles < pe.max_particles) {
          j = (pe.sequence + pe.num_particles) % pe.max_particles;
          const f32 angle = ((f32)rand()/RAND_MAX) * pe.res->angle - (pe.res->angle/2) + PI/2;
          pe.positions[j] = (glm::vec3)(pe.pose._world * glm::vec4(
                            glm::linearRand(glm::vec3(-1), glm::vec3(1)) 
                          * glm::vec3(pe.res->box[0], pe.res->box[1], pe.res->box[2]), 1));

          pe.velocities[j] = glm::normalize(glm::vec3(cos(angle),sin(angle),0))
                           * pe.res->speed;

          pe.scales[j] = glm::vec3(
            pe.res->scale_start[0], 
            pe.res->scale_start[1], 
            pe.res->scale_start[2]);

          pe.colors[j] = glm::vec4(
            pe.res->color_start[0] / 255, 
            pe.res->color_start[1] / 255, 
            pe.res->color_start[2] / 255,
            pe.res->color_start[3] / 255);

          pe.accu_spawn -= 1/pe.res->rate;

          ++pe.num_particles;
        }
      }
    }

    void shutdown(ParticleSystem &system)
    {
      Allocator &a = *system.emitters._data._allocator;
      IdLookupTable<ParticleEmitter>::Entry *e, *end = idlut::end(system.emitters);

      for (e = idlut::begin(system.emitters); e < end; e++)
        a.deallocate(e->value.positions);
    }
   
    u64 create(ParticleSystem &system, const ParticleResource *res)
    {
      ParticleEmitter e;
      Allocator &a = *system.emitters._data._allocator;

      e.accu_spawn    = 0;
      e.accu_despawn  = 0;
      e.hot           = false;
      e.num_particles = 0;
      e.max_particles = res->rate * res->lifespan;
      e.sequence      = 0;
      e.positions     = (glm::vec3*)a.allocate(PARTICLE_SIZE * e.max_particles);
      e.velocities    = e.positions  + e.max_particles;
      e.scales        = e.velocities + e.max_particles;
      e.colors        = (glm::vec4*)(e.scales + e.max_particles);
      e.res           = res;
      e.active        = false;
      e.color_factor  = glm::vec4(
        (res->color_end[0] - res->color_start[0]) / 255 / res->lifespan,
        (res->color_end[1] - res->color_start[1]) / 255 / res->lifespan,
        (res->color_end[2] - res->color_start[2]) / 255 / res->lifespan,
        (res->color_end[3] - res->color_start[3]) / 255 / res->lifespan
      );
      e.scale_factor  = glm::vec3(
        (res->scale_end[0] - res->scale_start[0]) / res->lifespan,
        (res->scale_end[1] - res->scale_start[1]) / res->lifespan,
        (res->scale_end[2] - res->scale_start[2]) / res->lifespan
      );

      return idlut::add(system.emitters, e);
    }

    void start(ParticleSystem &system, u64 emitter)
    {
      idlut::lookup(system.emitters, emitter)->active = true;
    }

    void stop(ParticleSystem &system, u64 emitter)
    {
      idlut::lookup(system.emitters, emitter)->active = false;
    }

    void destroy(ParticleSystem &system, u64 emitter)
    {
      Allocator       &a = *system.emitters._data._allocator;
      ParticleEmitter &e = *idlut::lookup(system.emitters, emitter);
      
      a.deallocate(e.positions);
      idlut::remove(system.emitters, emitter);
    }

    Pose &get_pose(ParticleSystem &system, u64 emitter)
    {
      return idlut::lookup(system.emitters, emitter)->pose;
    }

    void gather(ParticleSystem &system)
    {
      IdLookupTable<ParticleEmitter>::Entry *e, *end = idlut::end(system.emitters);
      u32 i, j;
      u32 rounded_num_particles;

      for (e = idlut::begin(system.emitters); e < end; e++) {
        u32 tex_size[2];
        tex_size[0] = tex_size[1] = next_pow2_u32((u32)ceil(sqrtf(e->value.num_particles)));
        if (tex_size[0] * tex_size[1] / 2 >= e->value.num_particles)
          tex_size[1] /= 2;

        ParticleData &data = *(ParticleData*)renderer::create_graphic(GRAPHIC_TYPE_PARTICLE, sizeof(ParticleData) + (tex_size[0] * tex_size[1] * DATA_SIZE), {0});

        data.id = e->id;
        data.sys = &system;
        data.tex_name   = e->value.res->texture_name;
        data.tex_region = e->value.res->texture_region;
        data.tex_size[0] = (i32)tex_size[0];
        data.tex_size[1] = (i32)tex_size[1];

        i = e->value.sequence;
        data.num_particles = Min(i + e->value.num_particles, e->value.max_particles) - i;

        glm::mat4 *poses  = particle_data::get_poses(data);
        glm::vec4 *colors = particle_data::get_colors(data);
        for (i = 0; i < data.num_particles; i++) {
          j = i + e->value.sequence;
          poses[i] = IDENTITY_MAT4;
          poses[i] = poses[i] * glm::translate(IDENTITY_MAT4, e->value.positions[j]);
          poses[i] = poses[i] * glm::scale(IDENTITY_MAT4, e->value.scales[j]);
          
          colors[i] = e->value.colors[j];
        }

        if (data.num_particles == e->value.num_particles)
          continue;

        for (j = 0; i < e->value.num_particles; i++, j++) {
          poses [i] = IDENTITY_MAT4;
          poses [i] = poses[i] * glm::translate(IDENTITY_MAT4, e->value.positions[j]);
          poses [i] = poses[i] * glm::scale(IDENTITY_MAT4, e->value.scales[j]);
          colors[i] = e->value.colors[j];
        }
        data.num_particles = e->value.num_particles;
      }
    }

    void draw(const Graphic *graphics, u32 num_items, RenderBuffer &render_buffer)
    {
      glEnable(GL_BLEND);
      glDisable(GL_DEPTH_TEST);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

      for (u32 i = 0; i < num_items; i++) {
        const ParticleData &data = *(const ParticleData*)renderer::get_data(graphics + i);
        if (!data.num_particles)
          continue;

        // init system texture if necessary
        if (!data.sys->poses_texture) {
          glGenTextures(1, &data.sys->poses_texture);
          glGenTextures(1, &data.sys->colors_texture);

          glBindTexture(GL_TEXTURE_2D, data.sys->poses_texture);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

          glBindTexture(GL_TEXTURE_2D, data.sys->colors_texture);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        // create cache if necessary
        if (!hash::has(data.sys->rendered_emitters, data.id)) {
          ParticleCache c;
          glm::vec3 vertices[4];
          glm::vec2 tex_coords[4];

          const TextureResource *res_texture = (TextureResource*)resource_manager::get(RESOURCE_TYPE_TEXTURE, data.tex_name);
          const TextureResource::Region *res_region = texture_resource::region(res_texture, data.tex_region);
          const TextureResource::Page *res_page = texture_resource::page(res_texture, res_region);

          i32 stride = res_region->rotated ? 1 : 0;
          i32 region_width = res_region->rotated ? res_region->height : res_region->width;
          i32 region_height = res_region->rotated ? res_region->width : res_region->height;

          tex_coords[(stride + 0) % 4] = glm::vec2((f32)res_region->x / res_page->width, (f32)(res_region->y + region_height) / (f32)res_page->height);
          tex_coords[(stride + 1) % 4] = glm::vec2((f32)res_region->x / res_page->width, (f32)res_region->y / (f32)res_page->height);
          tex_coords[(stride + 2) % 4] = glm::vec2((f32)(res_region->x + region_width) / (f32)res_page->width, (f32)res_region->y / (f32)res_page->height);
          tex_coords[(stride + 3) % 4] = glm::vec2((f32)(res_region->x + region_width) / (f32)res_page->width, (f32)(res_region->y + region_height) / (f32)res_page->height);
          
          const f32 original_half_w = (res_region->margin[3] + res_region->width  + res_region->margin[1]) * .5f;
          const f32 original_half_h = (res_region->margin[0] + res_region->height + res_region->margin[2]) * .5f;

          const f32 half_w = res_region->width * .5f;
          const f32 half_h = res_region->height * .5f;

          const f32 center_x = -original_half_w + res_region->margin[3] + half_w;
          const f32 center_y = +original_half_h - res_region->margin[0] - half_h;

          vertices[0] = glm::vec3(center_x - half_w, center_y + half_h, 0.f);
          vertices[1] = glm::vec3(center_x - half_w, center_y - half_h, 0.f);
          vertices[2] = glm::vec3(center_x + half_w, center_y - half_h, 0.f);
          vertices[3] = glm::vec3(center_x + half_w, center_y + half_h, 0.f);

          c.vertices   = 0;
          glGenBuffers (1, &c.vertices);
          glBindBuffer (GL_ARRAY_BUFFER, c.vertices);
          glBufferData (GL_ARRAY_BUFFER, 4 * sizeof (glm::vec3), vertices, GL_STATIC_DRAW);
          PRINT_GL_LAST_ERROR();

          c.tex_coords = 0;
          glGenBuffers (1, &c.tex_coords);
          glBindBuffer (GL_ARRAY_BUFFER, c.tex_coords);
          glBufferData (GL_ARRAY_BUFFER, 4 * sizeof (glm::vec2), tex_coords, GL_STATIC_DRAW);
          PRINT_GL_LAST_ERROR();

          c.vao = 0;
          glGenVertexArrays (1, &c.vao);
          glBindVertexArray (c.vao);
          glBindBuffer (GL_ARRAY_BUFFER, c.vertices);
          glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
          glBindBuffer (GL_ARRAY_BUFFER, c.tex_coords);
          glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
          PRINT_GL_LAST_ERROR();

          c.texture = renderer::get_texture(data.tex_name, data.tex_region);
          hash::set(data.sys->rendered_emitters, data.id, c);
        }

        const ParticleCache &c = *hash::get(data.sys->rendered_emitters, data.id);

        // render
        glBindVertexArray(c.vao);
        PRINT_GL_LAST_ERROR();

        glBindBuffer(GL_ARRAY_BUFFER, c.vertices);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
        PRINT_GL_LAST_ERROR();

        glBindBuffer(GL_ARRAY_BUFFER, c.tex_coords);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);
        PRINT_GL_LAST_ERROR();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, c.texture);

        glUniform2f(glGetUniformLocation(renderer::get_program_id(GRAPHIC_TYPE_PARTICLE), "data_size"), (f32)data.tex_size[0], (f32)data.tex_size[1]);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, data.sys->poses_texture);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, data.tex_size[0] * 4, data.tex_size[1], 0, GL_RGBA, GL_FLOAT, particle_data::get_poses(data));

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, data.sys->colors_texture);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, data.tex_size[0], data.tex_size[1], 0, GL_RGBA, GL_FLOAT, particle_data::get_colors(data));

        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, INDICES, data.num_particles);
      }
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, 0);
      glBindVertexArray(0);
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);

      PRINT_GL_LAST_ERROR();
    }
  }

  ParticleSystem::ParticleSystem(Allocator &a) : 
    emitters(a), rendered_emitters(a) {}
}