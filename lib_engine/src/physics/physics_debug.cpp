#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <runtime/types.h>
#include <runtime/temp_allocator.h>
#include <runtime/idlut.h>
#include <runtime/assert.h>
#include <runtime/hash.h>

#include <geometry/geometric_system.h>
#include <physics/physics_system.h>
//#include <culling/culling_system.h>

#include <engine/matrix.h>
#include "physics_debug.h"
#include "physics_wrapper.h"

namespace pge
{
  namespace physics_debug
  {
    static bool display = false;
    static i32 ppm;

    const Color PHYSICAL(42, 193, 28, 255);  // green : KINEMATIC and DYNAMIC
    const Color DYNAMIC(0, 255, 255, 255);   // turquoise
    const Color KINEMATIC(225, 0, 220, 255); // purple
    const Color SLEEPING(255, 0, 0, 255);    // red: DYNAMIC sleping state
    const Color STATIC(248, 202, 64, 255);   // legendary: not KINEMATIC and not DYNAMIC

    void show(bool value)
    {
      display = value;
    }

    void gather(PhysicsWorld &physics_world)
    {
      if (!display) return;

      ppm = physics_system::get_ppm();

      const Hash<Shape*>::Entry *e   = hash::begin(physics_world.actor_shapes);
      const Hash<Shape*>::Entry *end = hash::end(physics_world.actor_shapes);
      Color color;

      TempAllocator<2048> ta;
      Array<glm::vec3> *vertices = MAKE_NEW(ta, Array<glm::vec3>, ta);

      DecomposedMatrix dm;
      glm::mat4 world_matrix;

      for (; e < end; e++) {
        const ShapeInfo &shape_info = *hash::get(physics_world.shape_infos, (u64)e->value);
        Actor &actor = *idlut::lookup(physics_world.actors, shape_info.actor);

        if (!actor.kinematic && !actor.dynamic) {
          color = STATIC;
        } else if (!actor.kinematic && actor.dynamic) {
          color = physics::is_sleeping(actor.body) ? SLEEPING : DYNAMIC;
        } else {
          color = KINEMATIC;
        }

        f32 actor_z;
        {
          glm::vec3 actor_position;
          pose::get_world_translation(physics_system::get_pose(physics_world, shape_info.actor), actor_position);
          actor_z = actor_position.z;
        }

        physics::get_body_position(actor.body, (glm::vec2&)dm.translation);
        dm.translation.x *= ppm;
        dm.translation.y *= ppm;
        dm.scale    = IDENTITY_SCALE;
        dm.rotation = glm::quat(glm::vec3(0.f, 0.f, physics::get_body_rotation(actor.body)));

        compose_mat4(world_matrix, dm);
        

        switch (shape_info.type) {
          case SHAPE_TYPE_CIRCLE:
          { 
            glm::vec3 offset;
            physics::get_shape_position(e->value, (glm::vec2&)offset);
            offset *= (f32)ppm;

            f32 radius = physics::get_circle_radius(e->value) * ppm;
            glm::vec3 center(dm.translation + offset);
            center.z = actor_z;

            glm::vec3 circle_point = (glm::vec3)(world_matrix * glm::vec4(radius, 0.f, actor_z, 1.f)) + offset;
            ASSERT(center != circle_point);

            geometric_system::gather_circle(center, radius, color, false);
            geometric_system::gather_line(center, circle_point, color);
          }
            break;
          case SHAPE_TYPE_CHAIN:
          case SHAPE_TYPE_BOX:
          case SHAPE_TYPE_POLYGON:
          {
            u32 num_vertices = physics::get_num_vertices(e->value);
            const glm::vec2 *phys_vert = physics::get_vertices(e->value);

            array::reserve(*vertices, num_vertices);

            for (u32 i = 0; i < num_vertices; i++) {
              glm::vec4 p = glm::vec4(phys_vert[i].x * ppm, phys_vert[i].y * ppm, actor_z, 1.f);
              vertices->_data[i] = glm::vec3(world_matrix * p);
            }

            if (shape_info.type == SHAPE_TYPE_CHAIN)
              geometric_system::gather_chain(vertices->_data, num_vertices, color);
            else
              geometric_system::gather_polygon(vertices->_data, num_vertices, color);
          }
            break;
          default:
            XERROR("Shape type \"%d\" not handled!", shape_info.type);
        }
      }
      MAKE_DELETE(ta, Array<glm::vec3>, vertices);
    }
  }
}