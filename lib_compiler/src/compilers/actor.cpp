#include <runtime/types.h>
#include <runtime/trace.h>

#include <data/actor.h>

#include "compiler_types.h"
#include "compiler.h"

namespace
{
  using namespace pge;
}

namespace pge
{
  ActorCompiler::ActorCompiler(Allocator &a, StringPool &sp)
    : JsonCompiler(a, RESOURCE_TYPE_ACTOR, sp) {}

  bool ActorCompiler::compile(Work &w)
  {
    if (!load_json(w)) return false;

    ActorResource actor;

    u64 shapes = json::get_id(jsn, json::root(jsn), "shapes");

    actor._actor      = compiler::create_id_string(w, json::get_string(jsn, json::root(jsn), "template"));
    actor._num_shapes = json::size(jsn, shapes);

    fwrite(&actor, sizeof(ActorResource), 1, w.data);

    ActorResource::Shape shape;
    for (u32 i = 0; i < actor._num_shapes; i++)
    {

      const Json::Node &n = json::get_node(jsn, shapes, i);
      shape._instance_name = compiler::create_id_string(w, n.name);
      shape._template      = compiler::create_id_string(w, json::get_string(jsn, n.id, "template"));
      shape._material      = compiler::create_id_string(w, json::get_string(jsn, n.id, "material"));
      shape._shape         = compiler::create_reference(w, RESOURCE_TYPE_SHAPE, json::get_string(jsn, n.id, "shape"));
      compiler::read_json_pose(jsn, n.id, shape._pose);

      fwrite(&shape, sizeof(ActorResource::Shape), 1, w.data);
    }

    return true;
  }
}