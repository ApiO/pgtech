#include <Windows.h>
#include <limits>

#include <runtime/memory.h>
#include <runtime/array.h>
#include <runtime/idlut.h>
#include <runtime/json.h>
#include <runtime/file_system.h>
#include <runtime/temp_allocator.h>
//#include <runtime/murmur_hash.h>
#include <runtime/assert.h>

#include "level_handler.h"
#include "action_handler.h"
#include "selection_handler.h"
#include "project_handler.h"
#include "resource_handler.h"


// types & consts

using namespace pge;

namespace app
{
  using namespace handlers;

  const char *JSON_BEGIN = "{\n";
  const char *JSON_BEGIN_ROOT_TABLE = "\t\"%s\" : [\n\t\t";
  const char *JSON_END = "}";
  const char *JSON_END_ROOT_TABLE = "],\n";
  const char *JSON_END_LAST_ROOT_TABLE = "]\n";
  const char *FORMAT_ITEM_BEGIN = "{";
  const char *FORMAT_ITEM_END = "\n\t\t}%s";
  const char *FORMAT_RES_NAME = "\n\t\t\t\"name\" : \"%s\",";
  const char *FORMAT_RES_TRANSLATION = "\n\t\t\t\"translation\" : [%.6f, %.6f, %.6f]%s";
  const char *FORMAT_RES_ROTATION = "\n\t\t\t\"rotation\" : [%.6f, %.6f, %.6f]%s";
  const char *FORMAT_RES_SCALE = "\n\t\t\t\"scale\" : [%.6f, %.6f]%s";
  const char *FORMAT_RES_LAYER = "\n\t\t\t\"layer\" : %d";

  const char *FORMAT_LAYER_DATA     = "\n\t\t\t\"name\" : \"%s\",\n\t\t\t\"z\" : %.6f%s";
  const char *FORMAT_LAYER_VISIBLE  = "\n\t\t\t\"visible\" : false";
  const char *FORMAT_LAYER_SELECTED = "\n\t\t\t\"selected\" : true";

  inline void get_level_path(const char *level, char *path)
  {
    sprintf(path, "%s\\%s.pglevel", project->get_source_path(), level);
  }

  void save_resource(FILE *stream, const ResourceInfo &res_info, bool last)
  {
    char buf[512];

    // saves name
    fwrite(FORMAT_ITEM_BEGIN, strlen(FORMAT_ITEM_BEGIN)*sizeof(char), 1, stream);

    // saves name
    sprintf(buf, FORMAT_RES_NAME, res_info.name);
    fwrite(buf, strlen(buf)*sizeof(char), 1, stream);

    bool has_rotation = res_info.rotation.y != 0.f || res_info.rotation.z != 0.f;
    bool has_scale = res_info.scale.x != 1.f || res_info.scale.y != 1.f;
    bool has_layer = res_info.layer > -1;

    // saves position
    sprintf(buf, FORMAT_RES_TRANSLATION,
      res_info.position.x, res_info.position.y, res_info.position.z,
      has_rotation || has_scale || has_layer ? "," : "");
    fwrite(buf, strlen(buf)*sizeof(char), 1, stream);

    // saves rotation
    if (has_rotation){
      sprintf(buf, FORMAT_RES_ROTATION, 0.f, res_info.rotation.y, res_info.rotation.z,
        has_scale || has_layer ? "," : "");
      fwrite(buf, strlen(buf)*sizeof(char), 1, stream);
    }

    // saves scale
    if (has_scale){
      sprintf(buf, FORMAT_RES_SCALE, res_info.scale.x, res_info.scale.y,
        has_layer ? "," : "");
      fwrite(buf, strlen(buf)*sizeof(char), 1, stream);
    }

    // saves layer index
    if (has_layer) {
      sprintf(buf, FORMAT_RES_LAYER, res_info.layer);
      fwrite(buf, strlen(buf)*sizeof(char), 1, stream);
    }

    // saves item's end
    sprintf(buf, FORMAT_ITEM_END, last ? "" : ",");
    fwrite(buf, strlen(buf)*sizeof(char), 1, stream);
  }

  void save_resources(FILE *stream, const char *node_name, const IdLookupTable<ResourceInfo> &idlut, bool last)
  {
    if (idlut::size(idlut)){
      char tmp[64];
      sprintf(tmp, JSON_BEGIN_ROOT_TABLE, node_name);
      fwrite(tmp, strlen(tmp)*sizeof(char), 1, stream);

      for (u32 i = 0; i < idlut::size(idlut); i++) {
        const ResourceInfo &res_info = (idlut::begin(idlut) + i)->value;
        save_resource(stream, res_info, i == idlut::size(idlut) - 1);
      }

      const char *close = last ? JSON_END_LAST_ROOT_TABLE : JSON_END_ROOT_TABLE;
      fwrite(close, strlen(close)*sizeof(char), 1, stream);
    }
  }

  void save_layers(FILE *stream, const char *node_name, const Array<Layer*> &layers, bool last)
  {
    if (array::size(layers)){
      char buf[1024];

      sprintf(buf, JSON_BEGIN_ROOT_TABLE, node_name);
      fwrite(buf, strlen(buf)*sizeof(char), 1, stream);

      for (u32 i = 0; i < array::size(layers); i++){
        const Layer &layer = *layers[i];
        bool last = i == array::size(layers) - 1;

        fwrite(FORMAT_ITEM_BEGIN, strlen(FORMAT_ITEM_BEGIN)*sizeof(char), 1, stream);

        sprintf(buf, FORMAT_LAYER_DATA,
          layer.name, layer.z, !layer.visible || layer.selected ? "," : "");
        fwrite(buf, strlen(buf)*sizeof(char), 1, stream);

        if (!layer.visible)
          fwrite(FORMAT_LAYER_VISIBLE, strlen(FORMAT_LAYER_SELECTED)*sizeof(char), 1, stream);

        if (layer.visible && layer.selected)
          fwrite(FORMAT_LAYER_SELECTED, strlen(FORMAT_LAYER_SELECTED)*sizeof(char), 1, stream);

        // saves item's end
        sprintf(buf, FORMAT_ITEM_END, last ? "" : ",");
        fwrite(buf, strlen(buf)*sizeof(char), 1, stream);
      }

      const char *close = last ? JSON_END_LAST_ROOT_TABLE : JSON_END_ROOT_TABLE;
      fwrite(close, strlen(close)*sizeof(char), 1, stream);
    }
  }

  static inline void check_sub_directories(const char *root, const char *path)
  {
    const char *p = strchr(path, '/');
    
    if (!p) return;

    char rep[256] = "\0";
    strncat(rep, path, p - path);

    char sub_root[_MAX_PATH];
    sprintf(sub_root, "%s/%s", root, rep);

    if (!file_system::directory_exists(sub_root))
      file_system::directory_create(sub_root);

    check_sub_directories(sub_root, p + 1);
  }

}


// Header's definition

namespace app
{
  namespace handlers
  {
    LevelHandler *level;

    using namespace resource_handler;

    namespace level_handler
    {
      void init(Allocator &a)
      {
        level = MAKE_NEW(a, LevelHandler);
      }

      void shutdown(Allocator &a)
      {
        if (!level) return;

        level->close();
        MAKE_DELETE(a, LevelHandler, level);
        level = NULL;
      }
    }


    bool LevelHandler::load(const char *n)
    {
      if(strlen(name)) close();

      char level_path[MAX_PATH];
      get_level_path(n, level_path);

      bool res = file_system::file_exists(level_path);
      ASSERT(res);

      strcat(name, n);

      {
        TempAllocator4096 ta;
        StringPool sp(ta);
        Json json(ta, sp);
        u64  root = json::root(json);

        if (!json::parse_from_file(json, root, level_path, true))
        {
          char msg[2048];
          Allocator &a = memory_globals::default_allocator();
          Array<char*> errors(a);

          json::get_last_errors(json, errors);

          sprintf(msg, "Json error in file : %s\n", level_path);

          for (u32 i = 0; i < array::size(errors); i++){
            strcat(msg, errors[i]);
            if (i < array::size(errors) - 1)
              strcat(msg, "\n");
          }

          MessageBox(NULL, msg, "Json parse error", MB_OK | MB_ICONERROR);
          return false;
        }

        // load layers
        if (json::has(json, root, "layers"))
          resource->load_layers(json, json::get_node(json, root, "layers"));

        // load units
        if (json::has(json, root, "units"))
          resource->load_resources(json, json::get_node(json, root, "units"), RESOURCE_TYPE_UNIT);

        // load sprites
        if (json::has(json, root, "sprites"))
          resource->load_resources(json, json::get_node(json, root, "sprites"), RESOURCE_TYPE_SPRITE);
      }

      SET_MINT(loaded, 1u);
      SET_MINT(edited, 0u);
      SET_MINT(created, 1u);

      return true;
    }

    bool LevelHandler::save()
    {
      if (!BOOL(edited)) return true;

      char level_path[_MAX_PATH], backup[_MAX_PATH];
      get_level_path(name, level_path);

      if (file_system::file_exists(level_path)) {
        sprintf(backup, "%s.bkp", level_path);
        rename(level_path, backup);
      }
      else
        check_sub_directories(project->get_source_path(), name);
      

      FILE *stream = fopen(level_path, "wb");
      if (!stream) {
        char buf[512];
        sprintf(buf, "Level file not found.\nmissing: ", level_path);
        MessageBox(NULL, buf, "Json parse error", MB_OK | MB_ICONERROR);
        return false;
      }

      fwrite(JSON_BEGIN, strlen(JSON_BEGIN)*sizeof(char), 1, stream);

      const Array<Layer*> &layers  = resource->get_layers();
      const Resources     &units   = resource->get_units();
      const Resources     &sprites = resource->get_sprites();

      // writes layers
      save_layers(stream, "layers", layers, idlut::size(units) == 0 && idlut::size(sprites) == 0);

      // writes units
      save_resources(stream, "units", units, idlut::size(sprites) == 0);

      // writes sprites
      save_resources(stream, "sprites", sprites, true);

      fwrite(JSON_END, strlen(JSON_END)*sizeof(char), 1, stream);

      SET_MINT(edited, 0u);
      SET_MINT(created, 1u);

      fclose(stream);

      if (strlen(backup))
        file_system::delete_file(backup);

      return true;
    }

    void LevelHandler::close(void)
    {
      if (!BOOL(loaded)) return;

      resource->clear();

      name[0] = '\0';
      active_layer = -1;
      SET_MINT(loaded, 0u);
      SET_MINT(edited, 0u);
      SET_MINT(created, 0u);
        
      action->clear();
      selection->clear();
    }
    
    void LevelHandler::remove(void)
    {
      char level_path[_MAX_PATH];
      get_level_path(name, level_path);

      close();

      file_system::delete_file(level_path);
    }
    

    void LevelHandler::set_edited(void)
    {
      SET_MINT(edited, 1u);
    }


    void LevelHandler::set_active_layer(i32 index)
    {
      if (active_layer != -1)
        resource->set_layer_selected(active_layer, false);

      if (index != -1)
        resource->set_layer_selected(index, true);

      active_layer = index;
      set_edited();
    }
    
    i32 LevelHandler::get_active_layer_index(void)
    {
      return active_layer;
    }
    
    void LevelHandler::remove_layer(i32 id)
    {
      if (active_layer == id)
        active_layer = -1;
      
      resource->remove_layer(id);
    }



    bool LevelHandler::is_loaded(void)
    {
      return BOOL(loaded);
    }

    bool LevelHandler::is_edited(void)
    {
      return BOOL(edited);
    }

    bool LevelHandler::is_created(void)
    {
      return BOOL(created);
    }


    void LevelHandler::set_level_name(const char *n)
    {
      strcpy(name, n);
      SET_MINT(loaded, 1u);
    }

    const char *LevelHandler::get_level_name(void)
    {
      return name;
    }
    


  }
}