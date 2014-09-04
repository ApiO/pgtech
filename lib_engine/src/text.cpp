#include <runtime/murmur_hash.h>
#include <runtime/hash.h>
#include <glm/gtx/transform.hpp>
#include "text/text_system.h"
#include "resource/resource_manager.h"
#include "application.h"
#include "scene/scene_system.h"
#include "pose.h"

namespace pge
{
  namespace text
  {
    // Returns the unit that owns this text or 0 if there is none
    //u64 unit(u64 world, u64 text); TODO

    // Returns the text’s node index in the unit scene graph or -1 if no unit owns the text
    //i32 node(u64 world, u64 text); TODO

    // Returns the text’s width and height
    void get_size(u64 world, u64 text, glm::vec2 &v)
    {
      text_system::get_size(application::world(world).text_system, text, v);
    }

    void get_width(u64 world, u64 text, f32 &v)
    {
      text_system::get_width(application::world(world).text_system, text, v);
    }

    void get_height(u64 world, u64 text, f32 &v)
    {
      text_system::get_height(application::world(world).text_system, text, v);
    }

    // Sets the text’s string, font or alignment
    void set_string(u64 world, u64 text, const char *string)
    {
      World &w = application::world(world);
      text_system::set_string(w.text_system, text, string);
    }

    void set_font(u64 world, u64 text, const char *font)
    {
      World &w = application::world(world);
      text_system::set_font(w.text_system, text, (FontResource*)resource_manager::get(RESOURCE_TYPE_FONT, murmur_hash_32(font)));
    }

    void set_alignment(u64 world, u64 text, TextAlign align)
    {
      World &w = application::world(world);
      text_system::set_alignment(w.text_system, text, align);
    }

    void set(u64 world, u64 text, const char *font, const char *string, TextAlign align)
    {
      World &w = application::world(world);
      text_system::set(w.text_system, text, (FontResource*)resource_manager::get(RESOURCE_TYPE_FONT, murmur_hash_32(font)), string, align);
    }


    // Pose stuff    

    void get_world_position(u64 world, u64 text, glm::vec3 &v)
    {
      pose::get_world_translation(
        text_system::get_pose(application::world(world).text_system, text), v);
    }

    void get_world_rotation(u64 world, u64 text, glm::quat &q)
    {
      pose::get_world_rotation(
        text_system::get_pose(application::world(world).text_system, text), q);
    }

    void get_world_scale(u64 world, u64 text, glm::vec3 &v)
    {
      pose::get_world_scale(
        text_system::get_pose(application::world(world).text_system, text), v);
    }

    void get_world_pose(u64 world, u64 text, glm::mat4 &m)
    {
      pose::get_world_pose(
        text_system::get_pose(application::world(world).text_system, text), m);
    }


    void get_local_position(u64 world, u64 text, glm::vec3 &v)
    {
      pose::get_local_translation(
        text_system::get_pose(application::world(world).text_system, text), v);
    }

    void get_local_rotation(u64 world, u64 text, glm::quat &q)
    {
      pose::get_local_rotation(
        text_system::get_pose(application::world(world).text_system, text), q);
    }

    void get_local_scale(u64 world, u64 text, glm::vec3 &v)
    {
      pose::get_local_scale(
        text_system::get_pose(application::world(world).text_system, text), v);
    }

    void get_local_pose(u64 world, u64 text, glm::mat4 &m)
    {
      compose_mat4(m,
                   text_system::get_pose(application::world(world).text_system, text)
                   ._local);
    }

    void set_local_position(u64 world, u64 text, const glm::vec3 &v)
    {
      pose::set_local_translation(
        text_system::get_pose(application::world(world).text_system, text), v);
    }

    void set_local_rotation(u64 world, u64 text, const glm::quat &q)
    {
      pose::set_local_rotation(
        text_system::get_pose(application::world(world).text_system, text), q);
    }

    void set_local_scale(u64 world, u64 text, const glm::vec3 &v)
    {
      pose::set_local_scale(
        text_system::get_pose(application::world(world).text_system, text), v);
    }

    void set_local_pose(u64 world, u64 text, const glm::mat4 &m)
    {
      DecomposedMatrix dm;
      decompose_mat4(m, dm);
      pose::set_local_pose(
        text_system::get_pose(application::world(world).text_system, text), dm);
    }
  }
}