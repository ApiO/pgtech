#pragma once

#include <data/actor.h>
#include <engine/matrix.h>
#include "physics_system.h"

namespace pge
{
  namespace actor_resource
  {
    void register_type(void);
    void *load(FILE *file, u32 name, u32 size);
    void  patch_up(void *data, u32 name);
  }

  namespace actor_resource
  {
    inline void register_type(void)
    {
      resource_manager::register_type(RESOURCE_TYPE_ACTOR,
                                      &actor_resource::load,
                                      &resource_manager::default_bring_in,
                                      &actor_resource::patch_up,
                                      &resource_manager::default_bring_out,
                                      &resource_manager::default_unload);
    }

    inline void *load(FILE *file, u32 name, u32 size) {
      (void)name;
      (void)size;
      ActorResource ar;
      fread(&ar, sizeof(ActorResource), 1, file);

      // allocate enough memory to store the ActorData
      ActorData *ad = (ActorData*)memory_globals::default_allocator().allocate(sizeof(ActorData) + sizeof(ActorData::Shape) * ar._num_shapes);
      // init the data from the resource
      ad->num_shapes = ar._num_shapes;
      // store the actor template name into the actor template pointer.
      // the name will be translated into the corresponding template pointer during the patchup.
      ad->actor  = (ActorTemplate*)ar._actor;
      // shape memory is located directly after the ActorData memory
      ad->shapes = (ActorData::Shape*)(ad+1);

      // again, store shape reference names wich will be translated into pointer later
      ActorResource::Shape sr;
      for (u32 i = 0; i < ad->num_shapes; i++) {
        fread(&sr, sizeof(ActorResource::Shape), 1, file);
        ad->shapes[i]._instance_name = sr._instance_name;
        ad->shapes[i].material = (MaterialTemplate*)sr._material;
        ad->shapes[i].tpl      = (ShapeTemplate*)sr._template;
        ad->shapes[i].shape    = (ShapeData*)sr._shape;
        ad->shapes[i].pose.translation = glm::vec3(sr._pose.tx, sr._pose.ty, 0);
        ad->shapes[i].pose.rotation    = glm::quat(glm::vec3(sr._pose.rx, sr._pose.ry, sr._pose.rz));
        ad->shapes[i].pose.scale       = glm::vec3(sr._pose.sx, sr._pose.sy, 1);
      }

      return ad;
    }

    inline void patch_up(void *data, u32 name)
    {
      (void)name;
      ActorData *ad = (ActorData*)data;
      // translate the actor template name into a pointer
      ad->actor = (ActorTemplate*)&physics_system::get_actor_template((u32)ad->actor);
      // translate shape reference names into pointers
      for (u32 i = 0; i < ad->num_shapes; i++) {
        ad->shapes[i].material = (MaterialTemplate*)&physics_system::get_material_template((u32)ad->shapes[i].material);
        ad->shapes[i].tpl      = (ShapeTemplate*)&physics_system::get_shape_template((u32)ad->shapes[i].tpl);
        ad->shapes[i].shape    = (ShapeData*)resource_manager::get(RESOURCE_TYPE_SHAPE, (u32)ad->shapes[i].shape);
      }
    }
  }
}