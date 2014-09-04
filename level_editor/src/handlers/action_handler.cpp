#include "action_handler.h"

namespace app
{
  namespace handlers
  {

    void ActionHandler::register_action(ActionType type, char *data)
    {

    }

    void ActionHandler::update(void)
    {

    }

    void ActionHandler::clear(void)
    {
      array::clear(do_list);
      array::clear(undo_list);
    }
  }
}

namespace app
{

  namespace handlers
  {
    ActionHandler *action;

    namespace action_handler
    {
      void init(Allocator &a)
      {
        action = MAKE_NEW(a, app::handlers::ActionHandler, a);
      }

      void shutdown(Allocator &a)
      {
        if (!action) return;
        MAKE_DELETE(a, ActionHandler, action);
        action = NULL;
      }
    }
  }

}