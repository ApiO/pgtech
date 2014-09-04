#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <runtime/collection_types.h>
#include <engine/matrix.h>

namespace pge
{
  struct SceneGraph
  {
    struct Link {
      bool dirty_local;
      bool dirty_world;
      i32  parent;
    };
    bool              dirty;
    pge::u32          num_nodes;
    glm::mat4        *world_poses;
    DecomposedMatrix *local_poses;
    Link             *links;
  };

  struct SceneSystem : Tree<SceneGraph>
  {
    SceneSystem(Allocator &a);
    ~SceneSystem();
  };
}