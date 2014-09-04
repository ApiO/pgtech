#pragma once

#include <runtime/memory.h>
#include <application_types.h>

namespace app
{
  using namespace pge;

  namespace handlers
  {    
    class LevelHandler
    {
    public:
      LevelHandler();
      bool load   (const char *name);
      bool save   (void);
      void close  (void);
      void remove (void);

      void set_edited (void);

      void set_active_layer       (i32 index);
      i32  get_active_layer_index (void);
      void remove_layer           (i32 id);

      bool is_loaded  (void);
      bool is_edited  (void);
      bool is_created (void);

      const char *get_level_name(void);
      void        set_level_name(const char *name);

    private:
      char   name[_MAX_PATH];
      mint32 loaded;
      mint32 edited;
      mint32 created;
      i32    active_layer;
    };

    inline LevelHandler::LevelHandler(){
      name[0] = '\0';
      active_layer = -1;
      SET_MINT(loaded, 0u);
      SET_MINT(edited, 0u);
      SET_MINT(created, 0u);
    }

    namespace level_handler
    {
      void init     (Allocator &a);
      void shutdown (Allocator &a);
      /*
      u64  create_resource  (ResourceType type, const char *name, const glm::vec3 &position, const f32 &roll, bool flip, const glm::vec2 &scale);
      u64  get_resource_id  (ResourceType &type, u64 &engine_id);
      void destroy_resource (EditorResource &res_id);

      i32  add_layer(void);
      void remove_layer(i32 id);

      const Array<Layer*> &get_layers(void);
      const Layer         &get_layer(i32 index);
      void get_layer(const EditorResource &res, i32 &index);
      void set_layer(const EditorResource &res, const i32 index);

      void set_current_layer(i32 index);
      void set_edited(void);
      */
    }

    extern LevelHandler *level;
  }
}