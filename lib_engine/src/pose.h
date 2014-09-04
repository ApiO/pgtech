#pragma once

#include <runtime/types.h>
#include <engine/matrix.h>

namespace pge
{
  struct Pose
  {
    Pose();
    bool             _dirty;
    bool             _dirty_local;
    bool             _dirty_world;
    DecomposedMatrix _local;
    glm::mat4        _world;
  };

  namespace pose
  {
    void get_world_translation(const Pose &pose, glm::vec3 &v);
    void get_world_rotation(const Pose &pose, glm::quat &q);
    void get_world_scale(const Pose &pose, glm::vec3 &v);
    void get_world_pose(const Pose &pose, glm::mat4 &m);

    void get_local_translation(const Pose &pose, glm::vec3 &v);
    void get_local_rotation(const Pose &pose, glm::quat &q);
    void get_local_scale(const Pose &pose, glm::vec3 &v);
    void get_local_pose(const Pose &pose, DecomposedMatrix &dm);

    void set_local_translation(Pose &pose, const glm::vec3 &v);
    void set_local_rotation(Pose &pose, const glm::quat &q);
    void set_local_scale(Pose &pose, const glm::vec3 &v);
    void set_local_pose(Pose &pose, const DecomposedMatrix &dm);

    void set_world_pose(Pose &pose, const glm::mat4 &m);
    void transform_world_pose(Pose &pose, const glm::mat4 &m);

    bool is_dirty(Pose &pose);
    void update(Pose &pose);
  }

  inline Pose::Pose() : _dirty(false), _dirty_local(false), _dirty_world(false), _world(IDENTITY_MAT4) {}

  namespace pose 
  {

    inline void get_world_translation(const Pose &pose, glm::vec3 &v)
    {
      get_mat4_translation(pose._world, v);
    }

    inline void get_world_rotation(const Pose &pose, glm::quat &q)
    {
      DecomposedMatrix dm;
      decompose_mat4(pose._world, dm);
      q = dm.rotation;
    }

    inline void get_world_scale(const Pose &pose, glm::vec3 &v)
    {
      DecomposedMatrix dm;
      decompose_mat4(pose._world, dm);
      v = dm.scale;
    }

    inline void get_world_pose(const Pose &pose, glm::mat4 &m)
    {
      m = pose._world;
    }

    inline void get_local_translation(const Pose &pose, glm::vec3 &v)
    {
      v = pose._local.translation;
    }

    inline void get_local_rotation(const Pose &pose, glm::quat &q)
    {
      q = pose._local.rotation;
    }

    inline void get_local_scale(const Pose &pose, glm::vec3 &v)
    {
      v = pose._local.scale;
    }

    inline void get_local_pose(const Pose &pose, DecomposedMatrix &dm)
    {
      dm = pose._local;
    }


    inline void set_local_translation(Pose &pose, const glm::vec3 &v)
    {
      pose._local.translation = v;
      pose._dirty = true;
      pose._dirty_local = true;
    }

    inline void set_local_rotation(Pose &pose, const glm::quat &q)
    {
      pose._local.rotation = q;
      pose._dirty = true;
      pose._dirty_local = true;
    }

    inline void set_local_scale(Pose &pose, const glm::vec3 &v)
    {
      pose._local.scale = v;
      pose._dirty = true;
      pose._dirty_local = true;
    }

    inline void set_local_pose(Pose &pose, const DecomposedMatrix &dm)
    {
      pose._local = dm;
      pose._dirty = true;
      pose._dirty_local = true;
    }

    inline void set_world_pose(Pose &pose, const glm::mat4 &m)
    {
      pose._world  = m;
      pose._dirty = true;
    }

    inline void transform_world_pose(Pose &pose, const glm::mat4 &m)
    {
      pose._world  = m;
      pose._dirty = true;
      pose._dirty_world = true;
    }
    
    inline void update(Pose &pose)
    {
      if (!pose._dirty_local && !pose._dirty_world)
        return;

      if (pose._dirty_local && !pose._dirty_world) {
        compose_mat4(pose._world, pose._local);
      } else {
        glm::mat4 m;
        compose_mat4(m, pose._local);
        pose._world = pose._world * m;
      }
      pose._dirty_local = false;
      pose._dirty_world = false;
      pose._dirty = false;
    }

    inline bool is_dirty(Pose &pose) 
    {
      return pose._dirty_local || pose._dirty_world;
    }
  }
}