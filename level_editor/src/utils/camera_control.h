#pragma once

#include <engine/pge.h>
#include "camera.h"

namespace app
{
  namespace utils
  {
    namespace internal_camera_control
    {
      static glm::vec3 default_translation(0.f);
      static glm::vec3 default_rotation(0.f);
      static glm::vec3 last_rotation(0.f);
      static glm::vec3 last_position(0.f);
      static Camera    *cam = NULL;
      const  f32 PAD_SENSITIVITY = .7f;
      static i32 pad_index = -1;
    }

    namespace camera_control
    {
      using namespace pge;

      static void setup(Camera &camera, glm::vec3 &position)
      {
        using namespace internal_camera_control;

        cam = &camera;
        default_translation = position;
        last_position = position;

        for (u32 i = 0; i < MAX_NUM_PADS; i++)
          if (pad::active(i) && pad::num_buttons(i) < 15){
          pad_index = i;
          break;
          }
      }

      static void update(f64 dt)
      {
        using namespace internal_camera_control;

        if (pad_index == -1 || !pad::active(pad_index)) return;

        if (pad::button(pad_index, 0))
        {
          cam->set_position(default_translation);
          last_position = default_translation;

          cam->set_euler_angles(default_rotation);
          last_rotation = default_rotation;

          return;
        }

        glm::vec3 position = cam->get_position();
        glm::vec3 euler_angles = cam->get_euler_angles();

        // TRANSLATION: left stick
        if (pad::pressed(pad_index, 8)){
          cam->set_position(default_translation);
        }
        else {
          // X MOVES
          f32 left_stick_horiz = pad::axes(pad_index, 0);
          if (left_stick_horiz > PAD_SENSITIVITY){
            cam->move(CAMERA_MOVE_RIGHT, dt);
          }
          else if (left_stick_horiz < -PAD_SENSITIVITY){
            cam->move(CAMERA_MOVE_LEFT, dt);
          }

          // Y MOVES
          f32 left_stick_vert = pad::axes(pad_index, 1);
          if (left_stick_vert > PAD_SENSITIVITY){
            cam->move(CAMERA_MOVE_BACKWARD, dt);
          }
          else if (left_stick_vert < -PAD_SENSITIVITY){
            cam->move(CAMERA_MOVE_FORWARD, dt);
          }

          // Z Moves - trigger top
          if (pad::button(pad_index, 4)){
            cam->move(CAMERA_MOVE_DOWN, dt);
          }
          else if (pad::button(pad_index, 5)){
            cam->move(CAMERA_MOVE_UP, dt);
          }
        }

        // ROTATION: right stick
        if (pad::pressed(pad_index, 9)){
          cam->set_euler_angles(default_rotation);
        }
        else {
          // X
          bool x_change, y_change, z_change;
          x_change = y_change = z_change = false;

          f32 right_stick_horiz = pad::axes(pad_index, 3);
          if (right_stick_horiz > PAD_SENSITIVITY){
            euler_angles.x -= f32(dt * cam->get_rotation_speed());
            x_change = true;
          }
          else if (right_stick_horiz < -PAD_SENSITIVITY){
            euler_angles.x += f32(dt * cam->get_rotation_speed());
            x_change = true;
          }
          if (x_change)
            if (euler_angles.x <= -360.f){
            euler_angles.x += 360.f;
            }
            else if (euler_angles.x >= 360.f){
              euler_angles.x -= 360.f;
            }

            // Y
            f32 right_stick_vert = pad::axes(pad_index, 4);
            if (right_stick_vert > PAD_SENSITIVITY){
              euler_angles.y -= f32(dt * cam->get_rotation_speed());
              y_change = true;
            }
            else if (right_stick_vert < -PAD_SENSITIVITY){
              euler_angles.y += f32(dt * cam->get_rotation_speed());
              y_change = true;
            }
            if (y_change)
              if (euler_angles.y <= -360.f){
              euler_angles.y += 360.f;
              }
              else if (euler_angles.y >= 360.f){
                euler_angles.y -= 360.f;
              }

              // Z
              f32 triggers = pad::axes(pad_index, 2);
              if (triggers > PAD_SENSITIVITY){
                euler_angles.z -= f32(dt * cam->get_rotation_speed());
                z_change = true;
              }
              else if (triggers < -PAD_SENSITIVITY){
                euler_angles.z += f32(dt * cam->get_rotation_speed());
                z_change = true;
              }
              if (z_change)
                if (euler_angles.z <= -360.f){
                euler_angles.z += 360.f;
                }
                else if (euler_angles.z >= 360.f){
                  euler_angles.z -= 360.f;
                }
        }

        if (last_rotation != euler_angles)
        {
          cam->set_euler_angles(euler_angles);
          last_rotation = euler_angles;
        }

        if (last_position != position)
        {
          cam->set_position(position);
          last_position = position;
        }
      }
    }
  }
}