#include <runtime/assert.h>
#include <runtime/trace.h>
#include <runtime/idlut.h>
#include <runtime/hash.h>
#include <runtime/murmur_hash.h>

#include "resource_handler.h"
#include "level_handler.h"
#include "project_handler.h"

using namespace pge;


namespace app
{
  // privates
  namespace handlers
  {
    ResourceInfo & ResourceHandler::get_resource_info(const EditorResource &er)
    {
      Resources *res = NULL;
      ResourceMap *res_map = NULL;
      switch (er.type)
      {
      case RESOURCE_TYPE_UNIT:
        res = &resource->units;
        res_map = &resource->units_map;
        break;
      case RESOURCE_TYPE_SPRITE:
        res = &resource->sprites;
        res_map = &resource->sprites_map;
        break;
      default:
        XERROR("Not implemented yet...");
      }

      return *(ResourceInfo*)idlut::lookup(*res, er.id);
    }

    u64 ResourceHandler::create_resource(ResourceType type, const char *name, const glm::vec3 &position, const f32 &roll, bool flip, const glm::vec2 &scale, i32 layer_index)
    {
      level->set_edited();

      ResourceInfo item;
      item.name = name;
      item.type = type;
      item.position = position;
      item.layer = layer_index;
      item.rotation = glm::vec3(0, flip ? 180 : 0.f, roll);
      item.scale = glm::vec3(scale, 1.f);
      item.instance = MAX_U64;

      Resources *res = NULL;

      switch (item.type)
      {
      case RESOURCE_TYPE_UNIT:
        res = &units;
        break;
      case RESOURCE_TYPE_SPRITE:
        res = &sprites;
        break;
      default:
        XERROR("Not implemented yet...");
      }

      u64 id = idlut::add(*res, item);

      ResourceInfo &ri = *idlut::lookup(*res, id);
      ri.id = id;

      // link res to layer
      if (ri.layer != -1)
        array::push_back(layers[ri.layer]->resources, (EditorResource)ri);

      if (ri.layer == -1 || layers[ri.layer]->visible)
        spawn_resource(ri);

      return ri.instance;
    }

    void ResourceHandler::spawn_resource(ResourceInfo &ri)
    {
      ResourceMap *res_map = NULL;

      switch (ri.type)
      {
      case RESOURCE_TYPE_UNIT:
        res_map = &units_map;
        break;
      case RESOURCE_TYPE_SPRITE:
        res_map = &sprites_map;
        break;
      default:
        XERROR("Not implemented yet...");
      }

      // spawn res
      glm::quat rotation(glm::radians(ri.rotation));

      switch (ri.type)
      {
      case RESOURCE_TYPE_UNIT:
        ri.instance = world::spawn_unit(app_data.world, ri.name, ri.position, rotation, ri.scale);
        break;
      case RESOURCE_TYPE_SPRITE:
        ri.instance = world::spawn_sprite(app_data.world, ri.name, ri.position, rotation, ri.scale);
        break;
      default:
        XERROR("Not implemented yet...");
      }

      // map res
      hash::set(*res_map, ri.instance, ri.id);
    }

    void ResourceHandler::despawn_resource(ResourceInfo &ri)
    {
      ResourceMap *res_map = NULL;

      switch (ri.type)
      {
      case RESOURCE_TYPE_UNIT:
        res_map = &units_map;
        break;
      case RESOURCE_TYPE_SPRITE:
        res_map = &sprites_map;
        break;
      default:
        XERROR("Not implemented yet...");
      }

      // unspawn
      switch (ri.type)
      {
      case RESOURCE_TYPE_UNIT:
        world::despawn_unit(app_data.world, ri.instance);
        break;
      case RESOURCE_TYPE_SPRITE:
        world::despawn_sprite(app_data.world, ri.instance);
        break;
      default:
        XERROR("Not implemented yet...");
      }

      // unmap
      hash::remove(*res_map, ri.instance);
      ri.instance = MAX_U64;
    }

  }
}


namespace app
{
  namespace handlers
  {
    void ResourceHandler::clear(void)
    {
      // clears units
      for (u32 i = 0; i < idlut::size(resource->units); i++){
        u64 id = (idlut::begin(resource->units) + i)->value.instance;
        if (id == MAX_U64) continue;
        world::despawn_unit(app_data.world, id);
      }

      idlut::clear(resource->units);
      hash::clear(resource->units_map);

      // clears sprites
      for (u32 i = 0; i < idlut::size(resource->sprites); i++){
        u64 id = (idlut::begin(resource->sprites) + i)->value.instance;
        if (id == MAX_U64) continue;
        world::despawn_sprite(app_data.world, id);
      }

      idlut::clear(resource->sprites);
      hash::clear(resource->sprites_map);

      // clears layers
      pge::Allocator &a = memory_globals::default_allocator();
      Layer **item, **end = array::end(resource->layers);
      for (item = array::begin(resource->layers); item < end; item++)
        MAKE_DELETE(a, Layer, *item);
      array::clear(resource->layers);
    }

    // engine resource getters

    void ResourceHandler::get_pose(const EditorResource &er, glm::mat4 &pose)
    {
      u64 instance = get_resource_info(er).instance;
      switch (er.type)
      {
      case RESOURCE_TYPE_UNIT:
        unit::get_world_pose(app_data.world, instance, 0, pose);
        break;
      case RESOURCE_TYPE_SPRITE:
        sprite::get_world_pose(app_data.world, instance, pose);
        break;
      default:
        XERROR("Not implemented yet...");
      }
    }

    void ResourceHandler::get_position(const EditorResource &er, glm::vec3 &translation)
    {
      translation = get_resource_info(er).position;
    }

    void ResourceHandler::get_rotation(const EditorResource &er, f32 &roll, bool &flip)
    {
      const glm::vec3 &rotation = get_resource_info(er).rotation;
      roll = rotation.z;
      flip = rotation.y > 0.f;
    }

    void ResourceHandler::get_scale(const EditorResource &er, glm::vec3 &scale)
    {
      scale = get_resource_info(er).scale;
    }

    const Array<Layer*> &ResourceHandler::get_layers(void)
    {
      return layers;
    }

    const Resources &ResourceHandler::get_units(void)
    {
      return units;
    }

    const Resources &ResourceHandler::get_sprites(void)
    {
      return sprites;
    }

    void ResourceHandler::get_box(const EditorResource &er, Box &box)
    {
      u64 instance = get_resource_info(er).instance;
      switch (er.type)
      {
      case RESOURCE_TYPE_UNIT:
        unit::box(app_data.world, instance, box);
        break;
      case RESOURCE_TYPE_SPRITE:
        sprite::box(app_data.world, instance, box);
        break;
      default:
        XERROR("Not implemented yet...");
      }
    }

    // engine resource setters

    void ResourceHandler::set_position(const EditorResource &er, const glm::vec3 &position)
    {
      ResourceInfo &ri = get_resource_info(er);

      ri.position = position;

      switch (er.type)
      {
      case RESOURCE_TYPE_UNIT:
        unit::set_local_position(app_data.world, ri.instance, 0, position);
        break;
      case RESOURCE_TYPE_SPRITE:
        sprite::set_local_position(app_data.world, ri.instance, position);
        break;
      default:
        XERROR("Not implemented yet...");
      }
      level->set_edited();
    }

    void ResourceHandler::set_rotation(const EditorResource &er, const pge::f32 &roll, bool flip)
    {
      ResourceInfo &ri = get_resource_info(er);

      glm::vec3 rotation(0, flip ? 180.f : 0.f, roll);

      ri.rotation = rotation;

      glm::quat q(glm::radians(rotation));

      switch (er.type)
      {
      case RESOURCE_TYPE_UNIT:
        unit::set_local_rotation(app_data.world, ri.instance, 0, q);
        break;
      case RESOURCE_TYPE_SPRITE:
        sprite::set_local_rotation(app_data.world, ri.instance, q);
        break;
      default:
        XERROR("Not implemented yet...");
      }

      level->set_edited();
    }

    void ResourceHandler::set_scale(const EditorResource &er, const glm::vec3 &scale)
    {
      ResourceInfo &ri = get_resource_info(er);
      ri.scale = scale;

      switch (er.type)
      {
      case RESOURCE_TYPE_UNIT:
        unit::set_local_scale(app_data.world, ri.instance, 0, scale);
        break;
      case RESOURCE_TYPE_SPRITE:
        sprite::set_local_scale(app_data.world, ri.instance, scale);
        break;
      default:
        XERROR("Not implemented yet...");
      }
      level->set_edited();
    }

    // level methodes

    void ResourceHandler::load_layers(Json &json, const Json::Node &resources)
    {
      u64 node_id = resources.child;
      while (node_id != json::NO_NODE){
        const Json::Node &node = json::get_node(json, node_id);

        u32 index = add_layer();
        Layer &layer = *resource->layers[index];

        strcpy(layer.name, json::get_string(json, node_id, "name"));
        layer.z = (f32)json::get_number(json, node_id, "z");

        layer.visible  = json::get_bool(json, node_id, "visible", true);
        layer.selected = layer.visible ? json::get_bool(json, node_id, "selected", false) : false;

        if (layer.selected)
          level->set_active_layer(index);

        node_id = node.next;
      }
    }

    void ResourceHandler::load_resources(Json &json, const Json::Node &resources, const ResourceType type)
    {
      u64 node_id = resources.child;
      while (node_id != json::NO_NODE){
        const Json::Node &node = json::get_node(json, node_id);

        glm::vec3 position(0);
        {
          u64 id = json::get_id(json, node_id, "translation");
          position.x = (f32)json::get_number(json, id, 0);
          position.y = (f32)json::get_number(json, id, 1);
          position.z = (f32)json::get_number(json, id, 2);
        }

        glm::vec3 rotation(0.f);
        if (json::has(json, node_id, "rotation")){
          u64 id = json::get_id(json, node_id, "rotation");
          rotation.x = (f32)json::get_number(json, id, 0);
          rotation.y = (f32)json::get_number(json, id, 1);
          rotation.z = (f32)json::get_number(json, id, 2);
        }

        glm::vec2 scale(1.f);
        if (json::has(json, node_id, "scale")){
          u64 id = json::get_id(json, node_id, "scale");
          scale.x = (f32)json::get_number(json, id, 0);
          scale.y = (f32)json::get_number(json, id, 1);
        }

        const char *name = project->get_string(murmur_hash_32(json::get_string(json, node.id, "name")));
        bool flip = rotation.y > 0.f;

        i32 layer_index = json::has(json, node.id, "layer") ? json::get_integer(json, node.id, "layer") : -1;

        create_resource(type, name, position, rotation.z, flip, scale, layer_index);

        node_id = node.next;
      }
    }
    
    u64 ResourceHandler::create_resource(ResourceType type, const char *name, const glm::vec3 &position, const f32 &roll, bool flip, const glm::vec2 &scale)
    {
      return create_resource(type, name, position, roll, flip, scale, level->get_active_layer_index());
    }
    
    void ResourceHandler::destroy_resource(EditorResource &er)
    {
      Resources *res = NULL;
      ResourceMap *res_map = NULL;

      switch (er.type)
      {
      case RESOURCE_TYPE_UNIT:
        res = &units;
        res_map = &units_map;
        break;
      case RESOURCE_TYPE_SPRITE:
        res = &sprites;
        res_map = &sprites_map;
        break;
      default:
        XERROR("Not implemented yet...");
      }

      ResourceInfo &ri = *idlut::lookup(*res, er.id);

      if (ri.layer > -1){
        Layer &l = *layers[ri.layer];
        for (u32 i = 0; i < array::size(l.resources); i++){
          if (l.resources[i].id != er.id) continue;
          l.resources[i] = array::pop_back(l.resources);
          break;
        }
      }

      despawn_resource(ri);

      idlut::remove(*res, er.id);

      level->set_edited();
    }
    

    // layer stuff

    void ResourceHandler::set_layer_z(i32 id, f32 z)
    {
      Layer &layer = *layers[id];
      layer.z = z;

      EditorResource *er = NULL;
      Resources      *res = NULL;

      for (u32 i = 0; i < array::size(layer.resources); i++)
      {
        er = &layer.resources[i];

        switch (er->type)
        {
        case RESOURCE_TYPE_UNIT:
          res = &units;
          break;
        case RESOURCE_TYPE_SPRITE:
          res = &sprites;
          break;
        default:
          XERROR("Not implemented yet...");
        }

        glm::vec3 &position = idlut::lookup(*res, er->id)->position;

        position.z = z;

        switch (er->type)
        {
        case RESOURCE_TYPE_UNIT:
          unit::set_local_position(app_data.world, er->id, 0, position);
          break;
        case RESOURCE_TYPE_SPRITE:
          sprite::set_local_position(app_data.world, er->id, position);
          break;
        default:
          XERROR("Not implemented yet...");
        }

      }

      // change all children res z value
      level->set_edited();
    }

    void ResourceHandler::set_layer_name(i32 id, const char *name)
    {
      strcpy(layers[id]->name, name);
      level->set_edited();
    }

    void ResourceHandler::set_layer_selected(i32 id, bool value)
    {
      layers[id]->selected = value;
    }

    void ResourceHandler::set_layer_visibility(i32 id, bool display)
    {
      Layer &layer = *layers[id];
      layer.visible = display;
      level->set_edited();

      if (display){
        // show layer content
        for (u32 i = 0; i < array::size(layer.resources); i++){
          ResourceInfo &ri = get_resource_info(layer.resources[i]);
          spawn_resource(ri);
        }
      }
      else{
        // hide layer content
        for (u32 i = 0; i < array::size(layer.resources); i++){
          ResourceInfo &ri = get_resource_info(layer.resources[i]);
          despawn_resource(ri);
        }
      }
    }


    void ResourceHandler::get_resource_layer_index(const EditorResource &er, i32 &index)
    {
      index = get_resource_info(er).layer;
    }

    void ResourceHandler::set_resource_layer(const EditorResource &er, const i32 index)
    {
      level->set_edited();

      ResourceInfo &ri = get_resource_info(er);

      //removes link into the previous layer
      if (ri.layer > -1){
        Layer &l = *layers[ri.layer];
        for (u32 i = 0; i < array::size(l.resources); i++){
          if (l.resources[i].id != ri.id) continue;
          l.resources[i] = array::pop_back(l.resources);
          break;
        }
      }

      ri.layer = index;

      if (index == -1)return;

      Layer &new_layer = *layers[index];
      array::push_back(new_layer.resources, er);

      if (!new_layer.visible) {
        despawn_resource(ri);
        return;
      }

      glm::vec3 position;
      get_position(er, position);

      position.z = new_layer.z;
      set_position(er, position);
    }


    i32 ResourceHandler::add_layer(void)
    {
      level->set_edited();

      pge::Allocator &a = memory_globals::default_allocator();
      Layer *layer = MAKE_NEW(a, Layer, a);
      strcpy(layer->name, "Layer");
      layer->selected = false;
      layer->visible = true;
      layer->z = 0.f;

      array::push_back(layers, layer);

      return array::size(layers) - 1;
    }

    void ResourceHandler::remove_layer(i32 id)
    {
      level->set_edited();

      Array<EditorResource> &res = layers[id]->resources;

      // releases resources layer's link
      if (array::size(res)){
        const EditorResource *item, *end = array::end(res);
        for (item = array::begin(res); item < end; item++)
          get_resource_info(*item).layer = -1;
      }

      pge::Allocator &a = *units._data._allocator;;

      MAKE_DELETE(a, Layer, layers[id]);

      layers[id] = array::pop_back(layers);

      if (!array::size(layers))
        return;

      // updates resources layer's link index
      if (array::size(res)){
        const EditorResource *item, *end = array::end(res);
        for (item = array::begin(res); item < end; item++)
          get_resource_info(*item).layer = id;
      }
    }

    const Layer &ResourceHandler::get_layer(i32 index)
    {
      return *layers[index];
    }
  }
}


namespace app
{
  namespace handlers
  {
    ResourceHandler *resource;

    namespace resource_handler
    {
      void init(Allocator &a)
      {
        resource = MAKE_NEW(a, ResourceHandler, a);
      }

      void shutdown(Allocator &a)
      {
        MAKE_DELETE(a, ResourceHandler, resource);
        resource = NULL;
      }
    }
  }
}