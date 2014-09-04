#pragma once

#include <application_types.h>
#include <runtime/types.h>
#include <imgui/imgui.h>

namespace app
{
  namespace bases
  {
    class Form
    {
    public:
      Form();
      virtual void update(pge::f64 delta_time) = 0;
      virtual void draw(void) = 0;
      virtual void set_position(pge::i32 x, pge::i32 y) = 0;
      void enable(void);
      void disable(void);
      bool is_enable(void);
      bool is_visible(void);
      bool is_over(void);
    protected:
      pge::i32 x, y;
      pge::i32 width, height;
      mint32 _over;
      mint32 _enable;
      mint32 _visible;
    };

    inline Form::Form()
    {
      SET_MINT(_over, 0u);
      SET_MINT(_enable, 1u);
      SET_MINT(_visible, 1u);
    }

    inline void Form::enable()
    {
      SET_MINT(_enable, 1u);
    }

    inline void Form::disable()
    {
      SET_MINT(_enable, 0u);
    }

    inline bool Form::is_enable()
    {
      return BOOL(_enable);
    }

    inline bool Form::is_visible()
    {
      return BOOL(_visible);
    }

    inline bool Form::is_over()
    {
      return BOOL(_over);
    }
  }
}