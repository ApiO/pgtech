#include <runtime/assert.h>
#include <engine/pge.h>

#include "camera.h"


namespace app
{

  namespace utils
  {
    using namespace pge;

    Camera::Camera() : world(NULL) { }


    // GETTERS

    pge::f32 Camera::get_near_range()
    {
      return z_near;
    }

    pge::f32 Camera::get_far_range()
    {
      return z_far;
    }

    pge::f32 Camera::get_vertical_fov()
    {
      return vertical_fov;
    }

    u64 Camera::get_id()
    {
      return id;
    }

    CameraMode Camera::get_mode()
    {
      return mode;
    }

    glm::vec3 Camera::get_euler_angles()
    {
      return euler_angles;
    }

    glm::vec3 Camera::get_position()
    {
      return position;
    }

    glm::vec3 Camera::get_up()
    {
      return up;
    }

    glm::vec3 Camera::get_forward()
    {
      return forward;
    }

    pge::f32 Camera::get_move_speed()
    {
      return move_speed;
    }

    pge::f32 Camera::get_rotation_speed()
    {
      return glm::degrees(rotation_speed);
    }

    glm::vec3 Camera::get_look_at()
    {
      return look_at;
    }


    // SETTERS

    void Camera::set_mode(CameraMode m, f32 near, f32 far)
    {
      mode = m;
      camera::set_projection_type(world, id, m == CAMERA_MODE_OTRHO
        ? PROJECTION_ORTHOGRAPHIC
        : PROJECTION_PERSPECTIVE);

      z_near = near;
      z_far = far;

      camera::set_near_range(world, id, near);
      camera::set_far_range(world, id, far);

      dirty = true;
    }

    void Camera::set_euler_angles(const glm::vec3 &r)
    {
      euler_angles = r;
      dirty = true;
    }

    void Camera::set_position(const glm::vec3 &_p)
    {
      position = _p;
      camera::set_local_translation(world, id, position);
    }

    void Camera::set_near_range(f32 value)
    {
      z_near = value;
      camera::set_near_range(world, id, value);
    }

    void Camera::set_far_range(f32 value)
    {
      z_far = value;
      camera::set_far_range(world, id, value);
    }

    void Camera::set_up(const glm::vec3 &u)
    {
      up = u;
      dirty = true;
    }

    void Camera::set_forward(const glm::vec3 &f)
    {
      forward = f;
      dirty = true;
    }

    void Camera::set_move_speed(pge::f32 v)
    {
      move_speed = v;
    }

    void Camera::set_rotation_speed(pge::f32 v)
    {
      rotation_speed = glm::radians(v);
    }

    void Camera::set_look_at(const glm::vec3 &l)
    {
      look_at = l;
      dirty = true;
    }

    void Camera::set_orthographic_projection(f32 left, f32 right, f32 bottom, f32 top)
    {
      camera::set_orthographic_projection(world, id, left, right, bottom, top);
    }

    void Camera::set_vertical_fov(pge::f32 degree)
    {
      vertical_fov = degree;
      camera::set_vertical_fov(world, id, degree);
    }


    // FUNC

    void Camera::init(u64 w, CameraMode m, f32 width, f32 height)
    {
      resolution = glm::vec2(width, height);

      rfov = DEFAULT_FOV * (f32)(atan(1.0)*4.0) / 180.0f;
      f32 tz = resolution.y / (2 * tan(rfov / 2)); // z for 1:1 render scale

      world = w;
      mode = m;
      id = world::spawn_camera(w, width / height, glm::vec3(0, 0, tz), DEFAULT_ROTATION);

      reset();
    }

    void Camera::shutdown()
    {
      if (!world) return;
      world::despawn_camera(world, id);
    }

    void Camera::reset()
    {
      f32 tz = resolution.y / (2 * tan(rfov / 2));

      up = DEFAULT_UP;
      forward = DEFAULT_FORWARD;
      euler_angles = DEFAULT_EULER_ANGLES;
      move_speed = DEFAULT_MOVE_SPEED;
      rotation_speed = DEFAULT_ROTATION_SPEED;
      dirty = false;
      position = glm::vec3(0, 0, tz);
      z_near = DEFAULT_NEAR_RANGE;
      z_far = DEFAULT_FAR_RANGE;
      vertical_fov = DEFAULT_FOV;

      camera::set_projection_type(world, id, mode == CAMERA_MODE_OTRHO
        ? PROJECTION_ORTHOGRAPHIC
        : PROJECTION_PERSPECTIVE);

      camera::set_local_translation(world, id, position);
      camera::set_near_range(world, id, z_near);
      camera::set_far_range(world, id, z_far);
      camera::set_vertical_fov(world, id, vertical_fov);
    }

    void Camera::move(CameraDirection direction, pge::f64 delta_time)
    {
      f32 distance = f32(delta_time * move_speed);
      glm::vec3 n;
      glm::mat4 m;

      switch (mode)
      {
      case CAMERA_MODE_FREE:
        m = glm::translate(IDENTITY_MATRIX, position);
        m = m * glm::toMat4(glm::quat(glm::radians(euler_angles)));
        switch (direction)
        {
        case CAMERA_MOVE_UP:       n = up *  distance; break;
        case CAMERA_MOVE_DOWN:     n = up * -distance; break;
        case CAMERA_MOVE_RIGHT:    n = glm::cross(up, forward) * distance; break;
        case CAMERA_MOVE_LEFT:     n = glm::cross(forward, up) * distance; break;
        case CAMERA_MOVE_FORWARD:  n = forward * -distance; break;
        case CAMERA_MOVE_BACKWARD: n = forward *  distance; break;
        }
        m = glm::translate(m, n);
        position = (glm::vec3)(m * glm::vec4(0.f, 0.f, 0.f, 1.f));
        break;
      case CAMERA_MODE_OTRHO:
        switch (direction)
        {
        case CAMERA_MOVE_UP:       position.y += distance; break;
        case CAMERA_MOVE_DOWN:     position.y -= distance; break;
        case CAMERA_MOVE_LEFT:     position.x -= distance; break;
        case CAMERA_MOVE_RIGHT:    position.x += distance; break;
        case CAMERA_MOVE_FORWARD:  position.z += distance; break;
        case CAMERA_MOVE_BACKWARD: position.z -= distance; break;
        }
        break;
      case CAMERA_MODE_LOOK_AT:
        // on calcul la nouvelle translation 
        //en fonction du point observé(look_at/up/forward) et de la vitesse de déplacement
        XERROR("move CAMERA_MODE_LOOK_AT not implemented");
        break;
      }
      camera::set_local_translation(world, id, position);
    }

    void Camera::update()
    {
      if (!dirty) return;
      dirty = false;

      if (mode == CAMERA_MODE_LOOK_AT)
      {
        /*
        on update "euler_angles" en fonction du vec3 "look_at"
        */
      }

      // set quat en fonction du up et forward

      glm::quat q = glm::quat(glm::radians(euler_angles));
      camera::set_local_rotation(world, id, q);
    }

  }
}