#include <runtime/array.h>

#include <handlers/resource_handler.h>
#include <handlers/level_handler.h>
#include "forms.h"

using namespace app::handlers;

namespace app
{
  namespace forms
  {
    LayerForm::LayerForm() : form_scroll(0), add_layer(false), load_layer(false)
    {
      width = WINDOW_MAX_WIDTH;
    }

    void LayerForm::set_position(i32 _x, i32 _y)
    {
      x = _x;
      y = _y;
    }

    void LayerForm::update(f64 delta_time)
    {
      (void)delta_time;

      if (add_layer){
        EditorResource er;
        er.id   = resource->add_layer();
        er.type = RESOURCE_TYPE_LAYER;

        property->load(er);
        add_layer = false;
      }

      if (load_layer){
        EditorResource er;
        er.id = load_layer_index;
        er.type  = RESOURCE_TYPE_LAYER;

        property->load(er);
        load_layer_index = (u32)-1;
        load_layer = false;
      }
    }

    void LayerForm::draw(void)
    {
      height = screen.height - PROPERTY_WINDOW_HEIGHT - WINDOW_MARGIN * 3;

      bool over = imguiBeginScrollArea("Layers", x, y - height, width, height, &form_scroll);
      SET_MINT(_over, over ? 1u : 0u);

      imguiSeparatorLine();

      if (imguiButton("New", is_enable() && level->is_loaded())){
        add_layer = true;
      }

      i32 index = 0;
      const Array<Layer*> &layers = resource->get_layers();

      Layer * const *  item, *const * end = array::end(layers);
      char tmp[256];
      for (item = array::begin(layers); item < end; item++)
      {
        imguiSeparator();

        Layer &layer = **item;

        sprintf(tmp, "%s (%d)", layer.name, array::size(layer.resources));
        if (imguiCollapse(tmp, layer.selected ? "selected" : NULL, false, BOOL(_enable))){
          load_layer = true;
          load_layer_index = index;
        }

        index++;
      }

      imguiEndScrollArea();
    }

    void LayerForm::show(void) {}

    void LayerForm::hide(void) {}

  }
}


namespace app
{
  forms::LayerForm *layers;

  namespace forms
  {
    namespace layer_form
    {
      void init(Allocator &a)
      {
        layers = MAKE_NEW(a, LayerForm);
      }

      void shutdown(Allocator &a)
      {
        if (!layers) return;
        MAKE_DELETE(a, LayerForm, layers);
        layers = NULL;
      }
    }
  }
}