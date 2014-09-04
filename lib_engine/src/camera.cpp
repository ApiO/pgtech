#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <runtime/types.h>
#include <runtime/memory.h>
#include <runtime/collection_types.h>
#include <runtime/idlut.h>

#include <engine/pge.h>
#include <engine/matrix.h>
#include "pose.h"
#include "application.h"
// C++ API

namespace pge
{
  namespace camera
  {
    /*
    u64 unit(u64 cam)
    {
    return cam->unit;
    }

    u32 node(u64 cam)
    {
    return cam->node;
    }
    */

    void get_local_translation(u64 world, u64 cam, glm::vec3 &t)
    {
      pose::get_local_translation(idlut::lookup(application::world(world).camera_system, cam)->pose, t);
    }

    void get_local_rotation(u64 world, u64 cam, glm::quat &q)
    {
      pose::get_local_rotation(idlut::lookup(application::world(world).camera_system, cam)->pose, q);
    }

    void get_projection_type(u64 world, u64 cam, ProjectionType &type)
    {
      type = idlut::lookup(application::world(world).camera_system, cam)->projection_type;
    }

    void get_near_range(u64 world, u64 cam, f32 &near_range)
    {
      near_range = idlut::lookup(application::world(world).camera_system, cam)->near_range;
    }

    void get_far_range(u64 world, u64 cam, f32 &far_range)
    {
      far_range = idlut::lookup(application::world(world).camera_system, cam)->far_range;
    }

    void get_vertical_fov(u64 world, u64 cam, f32 &fov)
    {
      fov = idlut::lookup(application::world(world).camera_system, cam)->fov;
    }

    void get_transformation_matrix(u64 world, u64 camera, glm::mat4 &m)
    {
      m = idlut::lookup(application::world(world).camera_system, camera)->projection_view;
    }


    void set_local_translation(u64 world, u64 cam, const glm::vec3 &translation)
    {
      pose::set_local_translation(idlut::lookup(application::world(world).camera_system, cam)->pose, translation);
    }

    void set_local_rotation(u64 world, u64 cam, const glm::quat &q)
    {
      pose::set_local_rotation(idlut::lookup(application::world(world).camera_system, cam)->pose, q);
    }

    void set_projection_type(u64 world, u64 cam, ProjectionType type)
    {
      Camera &camera = *idlut::lookup(application::world(world).camera_system, cam);
      camera.projection_type = type;
      camera.update_projection = true;
    }

    void set_near_range(u64 world, u64 cam, f32 value)
    {
      Camera &camera = *idlut::lookup(application::world(world).camera_system, cam);
      camera.near_range = value;
      camera.update_projection = true;
    }

    void set_far_range(u64 world, u64 cam, f32 value)
    {
      Camera &camera = *idlut::lookup(application::world(world).camera_system, cam);
      camera.far_range = value;
      camera.update_projection = true;
    }

    void set_vertical_fov(u64 world, u64 cam, f32 value)
    {
      Camera &camera = *idlut::lookup(application::world(world).camera_system, cam);
      camera.fov = glm::radians(value);
      camera.update_projection = true;
    }


    void set_orthographic_projection(u64 world, u64 cam, f32 left, f32 right, f32 bottom, f32 top)
    {
      Camera &camera = *idlut::lookup(application::world(world).camera_system, cam);
      camera.left = left;
      camera.right = right;
      camera.bottom = bottom;
      camera.top = top;
      camera.update_projection = true;
    }

    void screen_to_world(u64 world, u64 camera, const glm::vec3 &p, glm::vec3 &w)
    {
      Camera &c = *idlut::lookup(application::world(world).camera_system, camera);
      glm::vec3 pos = p;
      i32 width, height;

      window::get_size(width, height);
      pos.y = (f32)height - pos.y;
      w = glm::unProject(pos, c.view, c.projection, glm::vec4(0.0f, 0.0f, width, height));
    }
  }
}