#include <runtime/types.h>
#include <runtime/trace.h>
#include <runtime/tree.h>
#include <engine/matrix.h>

#include "scene_system.h"

#define NODE_SIZE (sizeof(glm::mat4) + sizeof(DecomposedMatrix) + sizeof(SceneGraph::Link))

namespace
{
  using namespace pge;

  void update_sg(SceneGraph *graph, SceneGraph *parent)
  {
    SceneGraph::Link *lk = graph->links;
    glm::mat4        *wp = graph->world_poses;
    u32 i;

    graph->dirty |= parent->dirty;
    if (!graph->dirty) return;

    // update the root
    // if the root node or the linked node of the parent scene graph is dirty
    if (graph->links[0].parent >= 0)
      graph->links[0].dirty_local |= parent->links[graph->links[0].parent].dirty_local;

    if (graph->links[0].dirty_local) {
      // update the root world pose regarding the linked node of the parent scene graph
      compose_mat4(*wp, graph->local_poses[0]);
      if (graph->links[0].parent >= 0)
        *wp = parent->world_poses[lk->parent] * *wp;
    }

    for (i = 1; i < graph->num_nodes; i++) {
      // create pointers for convinience
      lk = graph->links + i;
      wp = graph->world_poses + i;

      // transfer the parent dirtyness to the node
      // if the node and the parent node are not dirty, skip
      lk->dirty_local |= graph->links[lk->parent].dirty_local;
      if (!lk->dirty_local) continue;

      // calculate the world pose concidering the local pose 
      // & the parent world pose
      compose_mat4(*wp, graph->local_poses[i]);
      *wp = graph->world_poses[lk->parent] * *wp;
    }

    /*
    for (u32 i = 0; i < graph->num_nodes; i++) {
    glm::vec4 translation =  graph->world_poses[i] * glm::vec4(0.f, 0.f, 0.f, 1.f);
    glm::quat rotation = glm::toQuat(graph->world_poses[i]);
    OUTPUT("node: %u\tT: [%.2f, %.2f, %.2f]\tR: [%.2f, %.2f, %.2f, %.2f]",
    i,
    translation.x, translation.y, translation.z,
    rotation.x, rotation.y, rotation.z, rotation.w);
    }
    */
  }

  void update_aux(Tree<SceneGraph> &scene_graphs, u64 graph, SceneGraph *parent)
  {
    tree::Iterator it;
    SceneGraph *child = tree::step(scene_graphs, it, graph);

    while (child) {
      update_sg(child, parent);

      if (it.child != tree::NO_NODE)
        update_aux(scene_graphs, it.child, child);

      // get the next unit
      child = tree::step(scene_graphs, it, it.next);
    }
  }
}

namespace pge
{
  static glm::mat4 sg_root_wp[] ={ IDENTITY_MAT4 };
  static DecomposedMatrix sg_root_lp[] ={ DecomposedMatrix() };
  static SceneGraph::Link sg_root_lk[] = { { false, false, -1 } };
  const SceneGraph SG_ROOT_GRAPH ={ false, 1u, sg_root_wp, sg_root_lp, sg_root_lk };

  SceneSystem::SceneSystem(Allocator &a) : Tree<SceneGraph>(a, SG_ROOT_GRAPH) {};

  SceneSystem::~SceneSystem()
  {
    const Tree<SceneGraph>::Entry *entry = tree::begin(*this);
    const Tree<SceneGraph>::Entry *end   = tree::end(*this);
    for (; entry < end; entry++) {
      if (entry->id != this->_root)
        _data._allocator->deallocate(entry->value.world_poses);
    }
  }

  namespace scene_system
  {
    void update(SceneSystem &s)
    {
      tree::Iterator it;
      SceneGraph *sg = tree::enter(s, it);
      update_aux(s, it.child, sg);
    }

    void finalize_update(SceneSystem &s)
    {
      Tree<SceneGraph>::Entry *entry = tree::begin(s),
        *end   = tree::end(s);
      for (; entry < end; entry++) {
        entry->value.dirty = false;
        for (u32 node_i = 0; node_i < entry->value.num_nodes; node_i++) {
          entry->value.links[node_i].dirty_local = false;
          entry->value.links[node_i].dirty_world = false;
        }
      }
    }

    void update(SceneSystem &s, u64 graph) {
      tree::Iterator it;
      SceneGraph *sg =  tree::step(s, it, graph);
      SceneGraph *pg = &tree::get(s, it.parent);
      update_sg(sg, pg);
      //for (u32 i = 0; i < sg->num_nodes; i++)
      //  sg->links[i].dirty_local = false;
    }

    u64 add(SceneSystem &s, const DecomposedMatrix *local_poses, const i32 *parents, u32 num_nodes)
    {
      SceneGraph sg;
      sg.num_nodes   = num_nodes;
      sg.world_poses = (glm::mat4*)s._data._allocator->allocate(NODE_SIZE * num_nodes);
      sg.local_poses = (DecomposedMatrix*)(sg.world_poses + num_nodes);
      sg.links       = (SceneGraph::Link*)(sg.local_poses + num_nodes);
      sg.dirty       = 1;

      for (u32 i = 0; i < num_nodes; i++) {
        sg.local_poses[i]  = local_poses[i];
        sg.links[i].parent = parents[i];
        sg.links[i].dirty_local = true;
        sg.links[i].dirty_world = false;
      }
      return tree::add(s, tree::root(s), sg);
    }

    void remove(SceneSystem &s, u64 graph)
    {
      SceneGraph &sg = tree::get(s, graph);
      s._data._allocator->deallocate(sg.world_poses);

      tree::Iterator it;
      tree::step(s, it, graph);

      u64 child_id = it.child;
      SceneGraph *child = tree::step(s, it, child_id);
      while (child) {
        unlink(s, child_id);
        child_id = it.next;
        child = tree::step(s, it, it.next);
      }

      tree::remove(s, graph);
    }

    void link(SceneSystem &s, u64 child, u64 parent, i32 node)
    {
      SceneGraph &sg = tree::get(s, child);
      sg.local_poses[0]  = DecomposedMatrix();
      sg.links[0].parent = node;
      sg.links[0].dirty_local = true;
      sg.dirty = true;
      tree::move(s, child, parent);
    }

    void unlink(SceneSystem &s, u64 graph)
    {
      SceneGraph &sg = tree::get(s, graph);
      sg.links[0].parent = -1;
      decompose_mat4(sg.world_poses[0], sg.local_poses[0]);
      tree::move(s, graph, tree::root(s));
    }

    glm::vec3 &local_translation(SceneSystem &s, u64 graph, i32 node)
    {
      return tree::get(s, graph).local_poses[node].translation;
    }

    glm::quat &local_rotation(SceneSystem &s, u64 graph, i32 node)
    {
      return tree::get(s, graph).local_poses[node].rotation;
    }

    glm::vec3 &local_scale(SceneSystem &s, u64 graph, i32 node)
    {
      return tree::get(s, graph).local_poses[node].scale;
    }

    DecomposedMatrix &local_pose(SceneSystem &s, u64 graph, i32 node)
    {
      return tree::get(s, graph).local_poses[node];
    }

    // MOCHE : faire un get par ref
    glm::vec3 world_translation(SceneSystem &s, u64 graph, i32 node)
    {
      glm::vec3 translation;
      get_mat4_translation(tree::get(s, graph).world_poses[node], translation);
      return translation;
    }

    // MOCHE : faire un get par ref
    glm::quat world_rotation(SceneSystem &s, u64 graph, i32 node)
    {
      DecomposedMatrix dm;
      decompose_mat4(tree::get(s, graph).world_poses[node], dm);
      return dm.rotation;
    }

    // MOCHE : faire un get par ref
    glm::vec3 world_scale(SceneSystem &s, u64 graph, i32 node)
    {
      DecomposedMatrix dm;
      decompose_mat4(tree::get(s, graph).world_poses[node], dm);
      return dm.scale;
    }

    glm::mat4 &world_pose(SceneSystem &s, u64 graph, i32 node)
    {
      return tree::get(s, graph).world_poses[node];
    }

    void set_world_pose(SceneSystem &s, u64 graph, i32 node, const glm::mat4 &m)
    {
      SceneGraph &sg = tree::get(s, graph);
      sg.world_poses[node] = m;
      for (u32 i = node + 1; i < sg.num_nodes; i++) {
        if (sg.links[i].parent == node)
          sg.links[i].dirty_local = true;
      }
      sg.links[node].dirty_world = true;
      sg.dirty = true;
    }

    void set_local_translation(SceneSystem &s, u64 graph, i32 node, const glm::vec3 &translation)
    {
      SceneGraph &sg = tree::get(s, graph);
      sg.local_poses[node].translation = translation;
      sg.links[node].dirty_local = true;
      sg.dirty = true;
    }

    void set_local_rotation(SceneSystem &s, u64 graph, i32 node, const glm::quat &rotation)
    {
      SceneGraph &sg = tree::get(s, graph);
      sg.local_poses[node].rotation = rotation;
      sg.links[node].dirty_local = true;
      sg.dirty = true;
    }

    void set_local_scale(SceneSystem &s, u64 graph, i32 node, const glm::vec3 &scale)
    {
      SceneGraph &sg = tree::get(s, graph);
      sg.local_poses[node].scale = scale;
      sg.links[node].dirty_local = true;
      sg.dirty = true;
    }

    void set_local_pose(SceneSystem &s, u64 graph, i32 node, const DecomposedMatrix &pose)
    {
      SceneGraph &sg = tree::get(s, graph);
      sg.local_poses[node] = pose;
      sg.links[node].dirty_local = true;
      sg.dirty = true;
    }

    void set_local_pose(SceneSystem &s, u64 graph, i32 node, const glm::mat4 &pose)
    {
      SceneGraph &sg = tree::get(s, graph);

      decompose_mat4(pose, sg.local_poses[node]);

      sg.links[node].dirty_local = true;
      sg.dirty = true;
    }

    bool changed(SceneSystem &s, u64 graph, i32 node)
    {
      SceneGraph &sg = tree::get(s, graph);
      return sg.links[node].dirty_local || sg.links[node].dirty_world;
    }
  }

}