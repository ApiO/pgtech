#include <Windows.h>
#include <locale.h>

#include <runtime/memory.h>
#include <runtime/hash.h>
#include <runtime/array.h>

#include "app_watcher.h"

namespace
{
  using namespace pge;

  const i32 MAX_NUM_ITEM = 3128;

  char buf[2048];

  const char *TYPE_TO_STRING[] =
  {
    "APP_INIT",
    "DELTA_TIME",
    "UPDATE_THREAD",
    "RENDER_THREAD",
    "MAIN_LOOP",
    "SPRITE_RENDER",
    "SPRITE_NUM_ITEMS",
    "TEXT_RENDER",
    "TEXT_NUM_ITEMS",
    "PRIMITIVE_RENDER",
    "PRIMITIVE_NUM_ITEMS",
    "PHYSICS_UPDATES",
    "PHYSICS_TO_SCENE",
    "SCENE_SYSTEM_UPDATE",
    "SCENE_TO_PHYSICS",
    "SCENE_SYSTEM_FINALIZE",
    "SCENE_UPDATE",
    "RENDER_WORLD",
    "ANIMATION_UPDATE",
    "ANIMATION_TO_SCENE",
    "GLFW_SWAP_BUFFER",
    "CAMERA_SYSTEM_UPDATE",
    "CULLING_SYSTEM_UPDATE",
    "SUB_SYSTEMS_UPDATE",
    "GAMEPLAY_INIT_CB",
    "GAMEPLAY_UPDATE_CB",
    "GAMEPLAY_RENDER_CB",
    "GAMEPLAY_RENDER_INIT_CB",
    "GAMEPLAY_RENDER_BEGIN_CB",
    "GAMEPLAY_RENDER_END_CB",
    "GAMEPLAY_RENDER_SHUTDOWN_CB"
  };

  inline void register_type(WatcherLog type, u32 size)
  {
    Array<f32> *arr = MAKE_NEW((*internal_app_watcher::elements->_data._allocator),
      Array<f32>,
      (*internal_app_watcher::elements->_data._allocator));
    array::reserve(*arr, size);
    hash::set(*internal_app_watcher::elements, (u64)type, arr);
  }

  void output(const char *msg, ...)
  {
    buf[0] = '\0';

    va_list args;
    va_start(args, msg);
    vsprintf(buf, msg, args);
    va_end(args);

    OutputDebugString(buf);
  }
}

namespace pge
{
  namespace internal_app_watcher
  {
    Hash<Array<f32>*> *elements = NULL;
  }
  namespace app_watcher
  {
    void init(u32 num_values, Allocator &a)
    {
      internal_app_watcher::elements = MAKE_NEW(a, Hash<Array<f32>*>, a);

      register_type(WL_APP_INIT, num_values);
      register_type(WL_DELTA_TIME, num_values);
      register_type(WL_UPDATE_THREAD, num_values);
      register_type(WL_RENDER_THREAD, num_values);
      register_type(WL_MAIN_LOOP, num_values);
      register_type(WL_SPRITE_RENDER, num_values);
      register_type(WL_SPRITE_NUM_ITEMS, num_values);
      register_type(WL_TEXT_RENDER, num_values);
      register_type(WL_TEXT_NUM_ITEMS, num_values);
      register_type(WL_PRIMITIVE_RENDER, num_values);
      register_type(WL_PRIMITIVE_NUM_ITEMS, num_values);
      register_type(WL_PHYSICS_UPDATES, num_values);
      register_type(WL_PHYSICS_TO_SCENE, num_values);
      register_type(WL_SCENE_SYSTEM_UPDATE, num_values);
      register_type(WL_SCENE_TO_PHYSICS, num_values);
      register_type(WL_SCENE_SYSTEM_FINALIZE, num_values);
      register_type(WL_SCENE_UPDATE, num_values);
      register_type(WL_RENDER_WORLD, num_values);
      register_type(WL_ANIMATION_UPDATE, num_values);
      register_type(WL_ANIMATION_TO_SCENE, num_values);
      register_type(WL_GLFW_SWAP_BUFFER, num_values);
      register_type(WL_CAMERA_SYSTEM_UPDATE, num_values);
      register_type(WL_CULLING_SYSTEM_UPDATE, num_values);
      register_type(WL_SUB_SYSTEMS_UPDATE, num_values);
      register_type(WL_GAMEPLAY_INIT_CB, num_values);
      register_type(WL_GAMEPLAY_UPDATE_CB, num_values);
      register_type(WL_GAMEPLAY_RENDER_CB, num_values);
      register_type(WL_GAMEPLAY_RENDER_INIT_CB, num_values);
      register_type(WL_GAMEPLAY_RENDER_BEGIN_CB, num_values);
      register_type(WL_GAMEPLAY_RENDER_END_CB, num_values);
      register_type(WL_GAMEPLAY_RENDER_SHUTDOWN_CB, num_values);
    }

    void shut_down(void)
    {
      const  Hash<Array<f32>*>::Entry *entry = hash::begin(*internal_app_watcher::elements),
        *eend = hash::end(*internal_app_watcher::elements);

      char *oldLocale = setlocale(LC_NUMERIC, NULL);
      setlocale(LC_NUMERIC, "French_Canada.1252");

      output("\n----------------------------------------------------------\n");
      output("\n                    MONITORING DATA\n");
      output("\n----------------------------------------------------------\n");
      for (; entry < eend; entry++)
      {
        //TODO: print des valeurs
        Array<f32> *arr = entry->value;
        f32 *b = array::begin(*arr),
          *e = array::end(*arr);

        output("%s\n", TYPE_TO_STRING[entry->key]);
        for (; b < e; b++)
          output("%.5f\t", *b);
        output("\n");

        MAKE_DELETE((*internal_app_watcher::elements->_data._allocator),
          Array<f32>, arr);
      }
      output("\n----------------------------------------------------------\n");
      setlocale(LC_NUMERIC, oldLocale);

      MAKE_DELETE((*internal_app_watcher::elements->_data._allocator),
        Hash<Array<f32>*>,
        internal_app_watcher::elements);
    }
  }
}
