#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <runtime/types.h>

namespace app
{
  namespace utils
  {
    using namespace pge;

    const f32  DEFAULT_ROTATION_SPEED = glm::radians(3.f * 60);
    const f32  DEFAULT_MOVE_SPEED = 30.f * 60;
    const f32  DEFAULT_FOV = 45.f;
    const f32  DEFAULT_NEAR_RANGE = 0.2f;
    const f32  DEFAULT_FAR_RANGE = 100001.f;
    const glm::vec3 DEFAULT_UP(0.f, 1.f, 0.f);
    const glm::vec3 DEFAULT_FORWARD(0.f, 0.f, 1.f);
    const glm::vec3 DEFAULT_EULER_ANGLES(0.f);
    const glm::vec3 DEFAULT_TRANSLATION(0.f, 0.f, 0.f);
    const glm::quat DEFAULT_ROTATION(1.f, 0.f, 0.f, 0.f);
    const glm::mat4 IDENTITY_MATRIX(1.f);

    enum CameraDirection {
      CAMERA_MOVE_UP,
      CAMERA_MOVE_DOWN,
      CAMERA_MOVE_LEFT,
      CAMERA_MOVE_RIGHT,
      CAMERA_MOVE_FORWARD,
      CAMERA_MOVE_BACKWARD
    };
    enum CameraMode
    {
      CAMERA_MODE_FREE,
      CAMERA_MODE_OTRHO,
      CAMERA_MODE_LOOK_AT
    };

    class Camera
    {
    public:
      Camera();

      void init(u64 world, CameraMode mode, f32 width, f32 height);

      void shutdown();

      void reset();

      // Sets the camera's mode
      void set_mode(CameraMode mode, f32 near, f32 far);

      // Sets the camera's orientation. Changes ignored if CAMERA_MODE_LOOK_AT is defined
      void set_euler_angles(const glm::vec3 &r);

      // Sets the camera's up vector
      void set_up(const glm::vec3 &up);

      // Sets the camera's forward vector
      void set_forward(const glm::vec3 &forward);

      // Sets the camera's move speed
      void set_move_speed(f32 v);

      // Sets the orthographic projection matrix. Use if CAMERA_MODE_ORTHO
      void set_orthographic_projection(f32 left, f32 right, f32 bottom, f32 top);

      // Sets the vertical angle of view of the camera. Changes ignored if CAMERA_MODE_ORTHO is defined
      void set_vertical_fov(f32 degree);

      // Sets the camera position. Position value is changed by function "move" too.
      void set_position(const glm::vec3 &position);

      // Sets the camera's rotation speed
      void set_rotation_speed(f32 v);

      // Defines the point's coordinates that the camera is looking at. Used if CAMERA_MODE_LOOK_AT is defined
      void set_look_at(const glm::vec3 &look_at);

      // Sets the near range value according to the forward vector
      void set_near_range(f32 value);

      // Sets the far range value according to the forward vector
      void set_far_range(f32 value);

      // Updates the camera's position depending on its mode
      void move(CameraDirection direction, f64 delta_time);

      // Applies all changes on camera, depending on its mode
      void update();

      u64        get_id();
      CameraMode get_mode();
      glm::vec3  get_euler_angles();
      glm::vec3  get_position();
      glm::vec3  get_up();
      glm::vec3  get_forward();
      f32        get_move_speed();
      f32        get_rotation_speed();
      glm::vec3  get_look_at();
      f32        get_near_range();
      f32        get_far_range();
      f32        get_vertical_fov();

    private:
      u64        id;
      u64        world;
      CameraMode mode;
      bool       dirty;
      glm::vec3  up;
      glm::vec3  forward;
      glm::vec3  euler_angles;
      glm::vec3  position;
      glm::vec3  look_at;
      glm::vec2  resolution;
      f32        move_speed;
      f32        rotation_speed;
      f32        z_near;
      f32        z_far;
      f32        rfov;
      f32        vertical_fov;
    };
  }
}