#pragma once

#include <mint_types.h>

namespace app
{
  namespace bases
  {
    class Focusable
    {
    public:
      Focusable();
      void set_focus(bool value);
      bool is_focused(void);
    protected:
      bool trigger_focus;
      bool trigger_focus_lost;
    private:
      virtual void on_focus_callback(void) = 0;
      virtual void on_focus_lost_callback(void) = 0;
      mint32 focus;
    };
  }

  namespace focusable
  {
    extern bases::Focusable *current_focus;

    inline void reset()
    {
      if (!focusable::current_focus) return;
      focusable::current_focus->set_focus(false);
      focusable::current_focus = NULL;
    }
  }

  namespace bases
  {
    inline Focusable::Focusable() : trigger_focus(false), trigger_focus_lost(false)
    {
      SET_MINT(focus, 0u);
    }

    inline bool Focusable::is_focused(void)
    {
      return BOOL(focus);
    }

    inline void Focusable::set_focus(bool value)
    {
      using namespace focusable;
      if (value) {
        if (current_focus && current_focus != this){
          current_focus->set_focus(false);
        }
        current_focus = this;
      }
      else if (current_focus && current_focus == this) {
        current_focus = NULL;
      }

      if (trigger_focus && value) on_focus_callback();
      if (trigger_focus_lost && !value) on_focus_lost_callback();

      SET_MINT(focus, value ? 1u : 0u);
    }
  }

}