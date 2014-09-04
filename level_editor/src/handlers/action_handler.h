#pragma once

#include <runtime/memory.h>
#include <runtime/collection_types.h>
#include <runtime/array.h>

namespace app
{
  using namespace pge;

  namespace handlers
  {
    enum ActionType
    {
      ACTION_TYPE_LEVEL_NEW = 0,
      ACTION_TYPE_LEVEL_SAVE,
      ACTION_TYPE_LEVEL_RELOAD,
      ACTION_TYPE_LEVEL_DELETE,
      ACTION_TYPE_LEVEL_CLOSE,
      ACTION_TYPE_SPAWN_UNIT,
      ACTION_TYPE_SPAWN_SPRITE,
      ACTION_TYPE_UNSPAWN_UNIT,
      ACTION_TYPE_UNSPAWN_SPRITE,
      ACTION_TYPE_LOAD_PROPERTIES,
      ACTION_TYPE_CREATE_LAYER,
      ACTION_TYPE_UNDO,
      ACTION_TYPE_REDO,
      ACTION_TYPE_QUIT
    };

    struct Action
    {
      ActionType type;
      char      *data;
    };

    typedef Array<Action> Actions;

    class ActionHandler
    {
    public:
      ActionHandler(Allocator &a);
      void register_action(ActionType type, char *data);
      void update(void);
      void clear(void);
    private:
      Actions do_list;
      Actions undo_list;
    };

    inline ActionHandler::ActionHandler(Allocator &a) : do_list(a), undo_list(a) {}

    namespace action_handler
    {
      void init(Allocator &a);
      void shutdown(Allocator &a);
    }

    extern ActionHandler *action;
  }
}