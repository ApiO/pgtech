#pragma once

#include <runtime/memory.h>
#include <runtime/json.h>
#include <runtime/array.h>
#include <application_types.h>

namespace app
{
  using namespace pge;

  namespace handlers
  {
    struct Layer
    {
      Layer(Allocator &a);
      char name[128];
      f32  z;
      bool visible;
      bool selected;
      Array<EditorResource> resources; //Resources idlut ID in level "Resources" container
    };

    inline Layer::Layer(Allocator &a) : resources(a){}

    struct ResourceInfo : public EditorResource
    {
      const char *name;
      glm::vec3   position;
      glm::vec3   rotation;
      glm::vec3   scale;
      i32         layer;
      u64         instance;
    };

    typedef IdLookupTable<ResourceInfo> Resources;
    typedef Hash<u64> ResourceMap;

    class ResourceHandler
    {
    public:
      ResourceHandler(Allocator &a);
      void clear    (void);
      
      void get_pose     (const EditorResource &er, glm::mat4 &pose);
      void get_position (const EditorResource &er, glm::vec3 &translation);
      void get_rotation (const EditorResource &er, f32 &roll, bool &flip);
      void get_scale    (const EditorResource &er, glm::vec3 &scale);
      void get_box      (const EditorResource &er, Box &box);

      void set_position (const EditorResource &er, const glm::vec3 &translation);
      void set_rotation (const EditorResource &er, const f32 &roll, bool flip);
      void set_scale    (const EditorResource &er, const glm::vec3 &scale);

      const Array<Layer*> &get_layers  (void);
      const Resources     &get_units   (void);
      const Resources     &get_sprites (void);

      u64  create_resource  (ResourceType type, const char *name, const glm::vec3 &position, const f32 &roll, bool flip, const glm::vec2 &scale);
      void destroy_resource (EditorResource &er);

      i32  add_layer      (void);
      void remove_layer   (i32 id);
      void load_layers    (Json &json, const Json::Node &resources);
      void load_resources (Json &json, const Json::Node &resources, const ResourceType type);

      void get_resource_layer_index (const EditorResource &er, i32 &index);
      const Layer &get_layer(i32 index);

      void set_resource_layer (const EditorResource &er, const i32 index);
      void set_layer_z          (i32 id, f32 z);
      void set_layer_name       (i32 id, const char *name);
      void set_layer_selected   (i32 id, bool value);
      void set_layer_visibility (i32 id, bool display);

    private:
      Array<Layer*> layers;
      Resources     units;
      Resources     sprites;
      ResourceMap   units_map;   // map engine instance ID as key with Resources idlut ID as value
      ResourceMap   sprites_map; // map engine instance ID as key with Resources idlut ID as value
      ResourceInfo &get_resource_info(const EditorResource &res_id);
      
      void spawn_resource   (ResourceInfo &ri);
      void despawn_resource (ResourceInfo &ri);
      u64  create_resource  (ResourceType type, const char *name, const glm::vec3 &position, const f32 &roll, bool flip, const glm::vec2 &scale, i32 layer_index);
    };

    inline ResourceHandler::ResourceHandler(Allocator &a) : 
      units(a), sprites(a), 
      units_map(a), sprites_map(a), 
      layers(a) {}
    
    namespace resource_handler
    {
      void init     (Allocator &a);
      void shutdown (Allocator &a);
    }

    extern ResourceHandler *resource;
  }
}