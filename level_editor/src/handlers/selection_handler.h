#pragma once

#include <runtime/memory.h>
#include <runtime/collection_types.h>
#include <runtime/array.h>

#include <application_types.h>

namespace app
{
  using namespace pge;

  namespace handlers
  {
    struct SelectionState : EditorResource
    {
      Box       box;
      glm::vec3 state_position;
      glm::mat4 pose;
    };

    class SelectionHandler
    {
    public:
      SelectionHandler (Allocator &a);
      void select_item (void);
      void clear       (void);
      void update      (f64 dt);
      void synchronize (void);
      void draw        (void);
    private:
      Array<SelectionState> items;
      Array<SelectionState> states;
      bool      dirty;
      bool      start_translation;
      bool      start_rotation;
      glm::vec2 state_position;

      void select_all (void);
      void remove     (void);
      void move       (void);
      void rotate     (void);
      void scale      (void);
      void reset_rotation_and_scale(void);
    };

    inline SelectionHandler::SelectionHandler(Allocator &a) : items(a), states(a), start_translation(false), start_rotation(false), dirty(false){}

    namespace selection_handler
    {
      void init(Allocator &a);
      void shutdown(Allocator &a);
    }

    extern SelectionHandler *selection;
  }
}