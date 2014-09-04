#include <runtime/types.h>
#include <runtime/trace.h>

#include <data/shape.h>

#include "compiler_types.h"
#include "compiler.h"

namespace
{
  using namespace pge;

  ShapeType get_shape_type(const Json &jsn, const char *resource_file)
  {
    const char *str = json::get_string(jsn, json::root(jsn), "type", "novalue");

    if (strcmp(str, "CIRCLE") == 0)   return SHAPE_TYPE_CIRCLE;
    if (strcmp(str, "CHAIN") == 0)    return SHAPE_TYPE_CHAIN;
    if (strcmp(str, "BOX") == 0)      return SHAPE_TYPE_BOX;
    if (strcmp(str, "POLYGON") == 0)  return SHAPE_TYPE_POLYGON;

    XERROR("Type \"%s\" not handled for attribute \"%s\" in file %s", str, resource_file);
#ifdef _DEBUG
    return (ShapeType)0;
#endif
  }
}

namespace pge
{
  ShapeCompiler::ShapeCompiler(Allocator &a, StringPool &sp)
    : JsonCompiler(a, RESOURCE_TYPE_SHAPE, sp) {}

  bool ShapeCompiler::compile(Work &w)
  {
    if (!load_json(w)) return false;

    ShapeResource shape;
    shape._type = get_shape_type(jsn, w.src);

    f32 value;
    u64 arr;

    switch (shape._type)
    {
      case SHAPE_TYPE_CIRCLE:
        shape._num_components = 1;
        fwrite(&shape, sizeof(ShapeResource), 1, w.data);
        value = (f32)json::get_number(jsn, json::root(jsn), "radius");
        fwrite(&value, sizeof(f32), 1, w.data);
        break;
      case SHAPE_TYPE_BOX:
        shape._num_components = 2;
        fwrite(&shape, sizeof(ShapeResource), 1, w.data);
        value = (f32)json::get_number(jsn, json::root(jsn), "width");
        fwrite(&value, sizeof(f32), 1, w.data);
        value = (f32)json::get_number(jsn, json::root(jsn), "height");
        fwrite(&value, sizeof(f32), 1, w.data);
        break;
      case SHAPE_TYPE_CHAIN:
      case SHAPE_TYPE_POLYGON:
        arr = json::get_id(jsn, json::root(jsn), "value");
        shape._num_components = json::size(jsn, arr);
        fwrite(&shape, sizeof(ShapeResource), 1, w.data);
        for (u32 i = 0; i < shape._num_components; i++)
        {
          value = (f32)json::get_number(jsn, arr, i);
          fwrite(&value, sizeof(f32), 1, w.data);
        }
        break;
      default:
        LOG("Unhandled ShapeType enum value %d", type);
        return false;
    }

    return true;
  }
}