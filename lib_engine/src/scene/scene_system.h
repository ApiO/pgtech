#pragma once

#include "scene_types.h"

namespace pge
{
  namespace scene_system
  {
    // updates the world poses of all scene graph nodes in the system
    // and keep the modified nodes dirty
    void update(SceneSystem &s);

    // mark all the scene graph nodes as not dirty
    void finalize_update(SceneSystem &s);

    // updates the world poses of a specific scene graph in the system 
    // and tag all its nodes as clean
    void update(SceneSystem &s, u64 graph);

    // adds a scene graph with 'num_nodes' nodes and returns its id
    // 'poses' specify the initial local poses of the nodes
    // 'parents' specify the parent index of the nodes
    // children nodes must appear after their parents
    u64  add(SceneSystem &s, const DecomposedMatrix *local_poses, const i32 *parents, u32 num_nodes);

    // removes and destroys the 'graph' from the system
    void remove(SceneSystem &s, u64 graph);

    // links the 'child' graph to the 'node' of the 'parent' graph
    void link(SceneSystem &s, u64 child, u64 parent, i32 node);

    // unlinks graph from parent to root
    void unlink(SceneSystem &s, u64 graph);

    // returns if the specified node value changed
    bool changed(SceneSystem &s, u64 graph, i32 node);

    // gets the world translation, rotation, scale or pose of the node at 'node' in the scene 'graph'
    glm::vec3  world_translation(SceneSystem &s, u64 graph, i32 node);
    glm::quat  world_rotation(SceneSystem &s, u64 graph, i32 node);
    glm::vec3  world_scale(SceneSystem &s, u64 graph, i32 node);
    glm::mat4 &world_pose(SceneSystem &s, u64 graph, i32 node);

    // directly sets the world pose of a node and tag it's direct children dirty
    // useful if the node is driven by a component like physical actors
    void set_world_pose(SceneSystem &s, u64 graph, i32 node, const glm::mat4 &m);

    // gets the local translation, rotation, scale or pose of the node at 'node' in the scene 'graph'
    glm::vec3 &local_translation(SceneSystem &s, u64 graph, i32 node);
    glm::quat &local_rotation(SceneSystem &s, u64 graph, i32 node);
    glm::vec3 &local_scale(SceneSystem &s, u64 graph, i32 node);
    DecomposedMatrix &local_pose(SceneSystem &s, u64 graph, i32 node);

    // sets the translation, rotation, scale or pose of the node at 'node' in the scene 'graph'
    void set_local_translation(SceneSystem &s, u64 graph, i32 node, const glm::vec3 &translation);
    void set_local_rotation(SceneSystem &s, u64 graph, i32 node, const glm::quat &rotation);
    void set_local_scale(SceneSystem &s, u64 graph, i32 node, const glm::vec3 &scale);
    void set_local_pose(SceneSystem &s, u64 graph, i32 node, const glm::mat4 &pose);
    void set_local_pose(SceneSystem &s, u64 graph, i32 node, const DecomposedMatrix &pose);
  }


}