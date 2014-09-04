#include <stdio.h>

#include <engine/pge.h>
#include <runtime/assert.h>
#include <runtime/array.h>
#include <runtime/trace.h>

#include <application_types.h>
#include <controls/input_control.h>
#include <handlers/resource_handler.h>
#include <handlers/level_handler.h>
#include <handlers/selection_handler.h>

#include "forms.h"

using namespace app::handlers;


#define TRANS_X 0
#define TRANS_Y 1
#define TRANS_Z 2
#define SCALE_X 0
#define SCALE_Y 1

namespace app
{
  using namespace controls;

  namespace forms
  {
    inline void draw_input(const i32 form_x, const i32 form_y, InputControl &input, i32 row_index)
    {
      input.set_position(form_x + 30, form_y + PROPERTY_WINDOW_HEIGHT - 46 - (row_index * (INPUT_HEIGHT + 4)));
      input.draw();
    }

    static void update_translation(void *user_data)
    {
      ComponentContext *ctx = (ComponentContext*)user_data;
      if (!strlen(ctx->translation[TRANS_X].get_value()) ||
        !strlen(ctx->translation[TRANS_Y].get_value()) ||
        !strlen(ctx->translation[TRANS_Z].get_value())) return;

      glm::vec3 translation;

      translation.x = (f32)atof(ctx->translation[TRANS_X].get_value());
      translation.y = (f32)atof(ctx->translation[TRANS_Y].get_value());
      translation.z = (f32)atof(ctx->translation[TRANS_Z].get_value());

      resource->set_position(*ctx->current_res, translation);
    }

    static void update_component_z(void *user_data)
    {
      ComponentContext *ctx = (ComponentContext*)user_data;
      update_translation(ctx);

      if (ctx->layer != -1){
        ctx->layer = -1;
        strcpy(ctx->layer_name, "Layer: none");
        resource->set_resource_layer(*ctx->current_res, -1);
      }
    }

    static void update_rotation(void *user_data)
    {
      ComponentContext *ctx = (ComponentContext*)user_data;
      if (!strlen(ctx->rotation.get_value())) return;

      f32 roll;
      bool flip;
      resource->get_rotation(*ctx->current_res, roll, flip);

      roll = (f32)atof(ctx->rotation.get_value());

      resource->set_rotation(*ctx->current_res, roll, flip);
    }

    static void update_scale(void *user_data)
    {
      ComponentContext *ctx = (ComponentContext*)user_data;
      if (!strlen(ctx->scale[SCALE_X].get_value()) ||
        !strlen(ctx->scale[SCALE_Y].get_value())) return;

      glm::vec3 scale(1.f);

      scale.x = (f32)atof(ctx->scale[SCALE_X].get_value());
      scale.y = (f32)atof(ctx->scale[SCALE_Y].get_value());

      resource->set_scale(*ctx->current_res, scale);
    }

    static void update_layer_name(void *user_data)
    {
      LayerContext *ctx = (LayerContext*)user_data;
      resource->set_layer_name((i32)ctx->current_res->id, ctx->name.get_value());
    }

    static void update_layer_z(void *user_data)
    {
      ((LayerContext*)user_data)->update_z = true;
    }
  }

  namespace forms
  {
    PropertyForm::PropertyForm()
    {
      height = PROPERTY_WINDOW_HEIGHT;
      width = WINDOW_MAX_WIDTH;
      form_scroll = 0;

      current_res.type = RESOURCE_TYPE_NONE;

      component_ctx.current_res = layer_ctx.current_res = &current_res;
      component_ctx.layer_list_scroll = 0;

      // [[Inits component context]]

      component_ctx.toggle_flip = false;
      component_ctx.display_layer_list = false;

      // init translation fields
      component_ctx.translation[TRANS_X].initialize("x", "0.000000", 0, 0, 40, 120, INPUT_FORMAT_FLOAT);
      component_ctx.translation[TRANS_X].set_validation_callback(update_translation, &component_ctx);

      component_ctx.translation[TRANS_Y].initialize("y", "0.000000", 0, 0, 40, 120, INPUT_FORMAT_FLOAT);
      component_ctx.translation[TRANS_Y].set_validation_callback(update_translation, &component_ctx);

      component_ctx.translation[TRANS_Z].initialize("z", "0.000000", 0, 0, 40, 120, INPUT_FORMAT_FLOAT);
      component_ctx.translation[TRANS_Z].set_validation_callback(update_component_z, &component_ctx);

      // init rotation field
      component_ctx.rotation.initialize("Rotation", "0.000000", 0, 0, 50 + IMGUI_PAD_WIDTH, 120, INPUT_FORMAT_FLOAT);
      component_ctx.rotation.set_validation_callback(update_rotation, &component_ctx);

      // init scale fields
      f32 default_scale = 1.f;
      component_ctx.scale[SCALE_X].initialize("x", "1.0", 0, 0, 40, 120, INPUT_FORMAT_FLOAT);
      component_ctx.scale[SCALE_X].set_validation_callback(update_scale, &component_ctx);
      component_ctx.scale[SCALE_X].set_min(&default_scale);

      component_ctx.scale[SCALE_Y].initialize("y", "1.0", 0, 0, 40, 120, INPUT_FORMAT_FLOAT);
      component_ctx.scale[SCALE_Y].set_validation_callback(update_scale, &component_ctx);
      component_ctx.scale[SCALE_Y].set_min(&default_scale);

      // [[Inits layer context]]

      layer_ctx.update_visibility = false;
      layer_ctx.update_z = false;
      layer_ctx.remove = false;

      layer_ctx.name.initialize("Name", "Layer", 0, 0, 40, 120, INPUT_FORMAT_TEXT);
      layer_ctx.name.set_validation_callback(update_layer_name, &layer_ctx);

      layer_ctx.z.initialize("z", "0.000000", 0, 0, 40, 120, INPUT_FORMAT_FLOAT);
      layer_ctx.z.set_validation_callback(update_layer_z, &layer_ctx);
    }

    void PropertyForm::refresh(void)
    {
      if (current_res.type == RESOURCE_TYPE_NONE) return;
      focusable::reset();
      load(current_res);
    }

    void PropertyForm::load_component(const EditorResource &er)
    {
      char value[32];

      // TRANSLATION
      glm::vec3 position;
      resource->get_position(er, position);

      sprintf(value, "%f", position.x);
      component_ctx.translation[TRANS_X].set_value(value, false);
      sprintf(value, "%f", position.y);
      component_ctx.translation[TRANS_Y].set_value(value, false);

      sprintf(value, "%f", position.z);
      component_ctx.translation[TRANS_Z].set_value(value, false);

      // LAYER
      i32 layer;
      resource->get_resource_layer_index(er, layer);
      component_ctx.layer = layer;

      if (layer == -1){
        strcpy(component_ctx.layer_name, "Layer: none");
      }
      else{
        sprintf(component_ctx.layer_name, "Layer: %s", resource->get_layer(layer).name);
      }

      // ROTATION & FLIP
      f32  roll;
      resource->get_rotation(er, roll, component_ctx.flip_y);
      sprintf(value, "%f", roll);
      component_ctx.rotation.set_value(value, false);

      // SCALE
      glm::vec3 scale(1.f);
      resource->get_scale(er, scale);

      sprintf(value, "%f", scale.x);
      component_ctx.scale[SCALE_X].set_value(value, false);
      sprintf(value, "%f", scale.y);
      component_ctx.scale[SCALE_Y].set_value(value, false);

      component_ctx.toggle_flip = false;
      component_ctx.display_layer_list = false;
    }

    void PropertyForm::load_layer(const u64 &id)
    {
      const Layer &layer = resource->get_layer((i32)id);

      layer_ctx.name.set_value(layer.name, false);

      char tmp[128];
      sprintf(tmp, "%.6f", layer.z);
      layer_ctx.z.set_value(tmp, false);

      layer_ctx.selected = layer.selected;
      layer_ctx.visible = layer.visible;

      layer_ctx.update_visibility = false;
      layer_ctx.select_layer = false;
      layer_ctx.update_z = false;
      layer_ctx.remove = false;
    }

    void PropertyForm::load(const EditorResource &er)
    {
      focusable::reset();

      current_res = er;
      switch (current_res.type)
      {
      case RESOURCE_TYPE_SPRITE:
      case RESOURCE_TYPE_UNIT:
        load_component(er);
        break;
      case RESOURCE_TYPE_LAYER:
        load_layer(er.id);
        break;
      default:
        XERROR("Not implemented yet...");
      }
    }

    void PropertyForm::unload(void)
    {
      if (current_res.type == RESOURCE_TYPE_NONE) return;
      current_res.type = RESOURCE_TYPE_NONE;
      focusable::reset();
    }

    void PropertyForm::set_position(i32 _x, i32 _y)
    {
      x = _x;
      y = _y;
    }

    void PropertyForm::show(void) { }

    void PropertyForm::hide(void) { }

  }
}

// update
namespace app
{
  namespace forms
  {
    void PropertyForm::update_component_form(f64 delta_time)
    {
      component_ctx.translation[TRANS_X].update(delta_time);
      component_ctx.translation[TRANS_Y].update(delta_time);
      component_ctx.translation[TRANS_Z].update(delta_time);
      component_ctx.rotation.update(delta_time);
      component_ctx.scale[SCALE_X].update(delta_time);
      component_ctx.scale[SCALE_Y].update(delta_time);

      if (component_ctx.toggle_flip) {
        f32 roll;
        resource->get_rotation(current_res, roll, component_ctx.flip_y);
        component_ctx.flip_y = !component_ctx.flip_y;
        resource->set_rotation(current_res, roll, component_ctx.flip_y);

        component_ctx.toggle_flip = false;
      }
    }

    void PropertyForm::update_layer_form(f64 delta_time)
    {
      layer_ctx.name.update(delta_time);
      layer_ctx.z.update(delta_time);

      if (layer_ctx.update_visibility){
        resource->set_layer_visibility((i32)current_res.id, layer_ctx.visible);
        if (!layer_ctx.visible) selection->clear();
        layer_ctx.update_visibility = false;
      }

      if (layer_ctx.update_z){
        resource->set_layer_z((i32)current_res.id, (f32)atof(layer_ctx.z.get_value()));
        layer_ctx.update_z = false;
      }

      if (layer_ctx.remove){
        level->remove_layer((i32)current_res.id);
        layer_ctx.remove = false;
        unload();
      }

      if (layer_ctx.select_layer){
        level->set_active_layer(layer_ctx.selected ? (i32)current_res.id : -1);
        resource->set_layer_selected((i32)current_res.id, layer_ctx.selected);
        layer_ctx.select_layer = false;
      }
    }

    void PropertyForm::update(f64 delta_time)
    {
      switch (current_res.type)
      {
      case RESOURCE_TYPE_UNIT:
      case RESOURCE_TYPE_SPRITE:
        update_component_form(delta_time);
        break;
      case RESOURCE_TYPE_LAYER:
        update_layer_form(delta_time);
        break;
      case RESOURCE_TYPE_NONE: break;
      default:
        XERROR("Not implemented yet...");
      }
    }
  }
}

// draw
namespace app
{
  namespace forms
  {
    void PropertyForm::draw_component_fields()
    {
      imguiIndent();

      imguiLabel("Translation:");
      imguiSeparator();
      imguiSeparator();
      imguiSeparator();
      imguiSeparator();
      imguiSeparator();

      imguiIndent();

      imguiSeparator();
      imguiSeparator();
      imguiSeparator();
      if (imguiItem(component_ctx.layer_name))
        component_ctx.display_layer_list = !component_ctx.display_layer_list;
      imguiUnindent();

      imguiSeparatorLine();

      imguiSeparator();
      imguiSeparator();
      imguiSeparator();

      imguiSeparatorLine();

      imguiLabel("Scale:");

      imguiSeparator();
      imguiSeparator();
      imguiSeparator();
      imguiSeparator();
      imguiSeparator();

      imguiSeparatorLine();

      if (imguiCheck("Flip", component_ctx.flip_y)){
        component_ctx.toggle_flip = true;
      }

      imguiUnindent();

      draw_input(x + IMGUI_PAD_WIDTH, y, component_ctx.translation[TRANS_X], 1);
      draw_input(x + IMGUI_PAD_WIDTH, y, component_ctx.translation[TRANS_Y], 2);
      draw_input(x + IMGUI_PAD_WIDTH, y, component_ctx.translation[TRANS_Z], 3);

      draw_input(x - 10, y, component_ctx.rotation, 6);

      draw_input(x + IMGUI_PAD_WIDTH, y, component_ctx.scale[SCALE_X], 9);
      draw_input(x + IMGUI_PAD_WIDTH, y, component_ctx.scale[SCALE_Y], 10);
    }

    void PropertyForm::draw_layer_list()
    {
      bool over = imguiBeginScrollArea("Select layer", x - WINDOW_MAX_WIDTH - WINDOW_MARGIN * 2, y, width, height, &component_ctx.layer_list_scroll);
      SET_MINT(_over, over ? 1u : 0u);

      if (imguiItem("[ None ]", component_ctx.layer != -1)) {
        strcpy(component_ctx.layer_name, "Layer: none");

        resource->set_resource_layer(current_res, -1);
        component_ctx.layer = -1;

        component_ctx.display_layer_list = false;
      }

      const Array<Layer*> &layers = resource->get_layers();
      for (u32 i = 0; i < array::size(layers); i++){
        if (imguiItem(layers[i]->name, (u32)component_ctx.layer != i))
        {
          sprintf(component_ctx.layer_name, "Layer: %s", layers[i]->name);

          resource->set_resource_layer(current_res, i);
          component_ctx.layer = i;

          char buf[16];
          sprintf(buf, "%.6f", layers[i]->z);
          component_ctx.translation[TRANS_Z].set_value(buf);

          component_ctx.display_layer_list = false;
        }
      }


      imguiEndScrollArea();
    }

    void PropertyForm::draw_layer_fields()
    {
      imguiIndent();
      imguiSeparator();
      imguiSeparator();
      imguiSeparator();
      imguiSeparator();
      imguiSeparator();

      if (imguiCheck("Select", layer_ctx.selected, is_enable() && layer_ctx.visible)){
        layer_ctx.selected = !layer_ctx.selected;
        layer_ctx.select_layer = true;
      }

      if (imguiCheck("Visible", layer_ctx.visible, is_enable())){
        layer_ctx.visible = !layer_ctx.visible;
        layer_ctx.update_visibility = true;

        if (!layer_ctx.visible && layer_ctx.selected){
          layer_ctx.selected = false;
          layer_ctx.select_layer = true;
        }
      }

      imguiUnindent();

      if (imguiButton("Delete"))
      {
        const Layer &layer = resource->get_layer((i32)layer_ctx.current_res->id);
        if (array::size(layer.resources)){
          char tmp[512];
          sprintf(tmp, "You are trying to delete layer \"%s\".\nContinue anyway ?", layer.name);
          if (MessageBox(NULL, tmp, "Warning", MB_OKCANCEL | MB_ICONWARNING) == IDOK)
            layer_ctx.remove = true;
        }
        else
          layer_ctx.remove = true;
      }

      draw_input(x, y, layer_ctx.name, 0);
      draw_input(x, y, layer_ctx.z, 1);
    }

    void PropertyForm::draw(void)
    {
      bool sub_window = false;

      bool over = imguiBeginScrollArea("Properties", x, y, width, height, &form_scroll);
      SET_MINT(_over, over ? 1u : 0u);

      imguiSeparatorLine();

      switch (current_res.type)
      {
      case RESOURCE_TYPE_UNIT:
      case RESOURCE_TYPE_SPRITE:
        draw_component_fields();
        sub_window = component_ctx.display_layer_list;
        break;
      case RESOURCE_TYPE_LAYER:
        draw_layer_fields();
        break;
      case RESOURCE_TYPE_NONE: break;
      default:
        XERROR("Not implemented yet...");
      }

      imguiEndScrollArea();

      if (sub_window) draw_layer_list();
    }

  }
}

namespace app
{
  forms::PropertyForm *property;

  namespace forms
  {
    namespace property_form
    {
      void init(Allocator &a)
      {
        property = MAKE_NEW(a, PropertyForm);
      }

      void shutdown(Allocator &a)
      {
        if (!property) return;
        MAKE_DELETE(a, PropertyForm, property);
        property = NULL;
      }
    }
  }
}