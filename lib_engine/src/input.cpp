#include <GL/glew.h>
#include <GL/GL.h>
#include <GLFW/glfw3.h>

#include <runtime/types.h>
#include <runtime/collection_types.h>
#include <runtime/array.h>
#include <runtime/hash.h>
#include <runtime/trace.h>

#include "application_types.h"
#include "input_system.h"

namespace
{
  using namespace pge;
  
  enum PadState
  {
    PAD_CONNECTED = 0,
    PAD_DISCONNECTED
  };

  struct ButtonStates
  {
    ButtonStates() : current(false), previous(false){}
    ButtonStates(bool cur, bool prev) : current(cur), previous(prev){}
    bool current;
    bool previous;
  };

  struct WheelScrool
  {
    i32  value;
    i32  cb_value;
    bool updated;
  };

  struct Pad
  {
    Pad() : current(PAD_DISCONNECTED), previous(PAD_DISCONNECTED),
      num_buttons(0), name(0), button_states(0){}
    char    *name;
    PadState current;
    PadState previous;
    i32      num_buttons;
    ButtonStates *button_states;
  };

  struct InputSystem
  {
    InputSystem(Allocator &a) : pads(a), keyboard_buttons(0), mouse_buttons(0){}
    ButtonStates *keyboard_buttons;
    ButtonStates *mouse_buttons;
    WheelScrool   mouse_wheel_scroll;
    Array<Pad>    pads;
  };

  static GLFWwindow  *w = NULL;
  static InputSystem *s = NULL;

  void cb_mouse_wheel(GLFWwindow*, f64, f64 y_offset)
  {
    s->mouse_wheel_scroll.cb_value = (i32)y_offset;
    s->mouse_wheel_scroll.updated  = false;
  }
}

namespace pge
{
  const u32 MAX_NUM_PADS         = GLFW_JOYSTICK_LAST + 1;
  const u32 MOUSE_NUM_BUTTONS    = GLFW_MOUSE_BUTTON_LAST + 1;
  const u32 KEYBOARD_NUM_BUTTONS = GLFW_KEY_LAST + 1;

  namespace input_system
  {
    void init(void *win, Allocator &a)
    {
      w = (GLFWwindow*)win;
      s = MAKE_NEW(a, InputSystem, a);

      ButtonStates default_state;

      // init keyboard buttons
      s->keyboard_buttons = (ButtonStates*)a.allocate(sizeof(ButtonStates)* KEYBOARD_NUM_BUTTONS);
      for (u32 i = 0; i < KEYBOARD_NUM_BUTTONS; i++)
        *(s->keyboard_buttons + i) = default_state;

      // init mouse buttons
      s->mouse_buttons = (ButtonStates*)a.allocate(sizeof(ButtonStates)* MOUSE_NUM_BUTTONS);
      for (u32 i = 0; i < MOUSE_NUM_BUTTONS; i++)
        *(s->mouse_buttons + i) = default_state;

      glfwSetScrollCallback(w, cb_mouse_wheel);
      s->mouse_wheel_scroll.value    = 0;
      s->mouse_wheel_scroll.cb_value = 0;
      s->mouse_wheel_scroll.updated  = true;

      // init pad buttons
      array::reserve(s->pads, MAX_NUM_PADS);
      for (u32 i = 0; i < MAX_NUM_PADS; i++)
        array::push_back(s->pads, Pad());

      update();
    }

    void update()
    {
      Allocator &a = *s->pads._allocator;

      // check pads connection & updates buttons
      for (u32 i = 0; i < MAX_NUM_PADS; i++) {
        Pad &pad = s->pads[i];

        pad.previous = pad.current;
        pad.current = glfwJoystickPresent(i) == GL_TRUE
          ? PAD_CONNECTED : PAD_DISCONNECTED;

        if (pad.current == PAD_DISCONNECTED) continue;

        const unsigned char* states = glfwGetJoystickButtons(i, &pad.num_buttons);

        if (pad.current == PAD_CONNECTED && pad.previous == PAD_DISCONNECTED) {
          if (pad.name) {
            a.deallocate(pad.name);
            a.deallocate(pad.button_states);
          }

          const char *name = glfwGetJoystickName(i);
          pad.name = (char*)a.allocate(sizeof(char)*strlen(name));
          strcpy(pad.name, name);

          pad.button_states = (ButtonStates*)a.allocate(sizeof(ButtonStates)*pad.num_buttons);
          for (i32 j = 0; j < pad.num_buttons; j++)
            pad.button_states[j] = ButtonStates(states[j] == 1, false);
        } else
          for (i32 j = 0; j < pad.num_buttons; j++) {
            pad.button_states[j].previous = pad.button_states[j].current;
            pad.button_states[j].current  = states[j] == 1;
          }
      }

      // update keyborad buttons
      for (u32 i = 0; i < KEYBOARD_NUM_BUTTONS; i++) {
        s->keyboard_buttons[i].previous = s->keyboard_buttons[i].current;
        s->keyboard_buttons[i].current  = glfwGetKey(w, i) == GLFW_PRESS;
      }

      // update mouse buttons
      for (u32 i = 0; i < MOUSE_NUM_BUTTONS; i++) {
        s->mouse_buttons[i].previous = s->mouse_buttons[i].current;
        s->mouse_buttons[i].current  = glfwGetMouseButton(w, i) == GLFW_PRESS;
      }

      if (!s->mouse_wheel_scroll.updated)
      {
        s->mouse_wheel_scroll.value   = s->mouse_wheel_scroll.cb_value;
        s->mouse_wheel_scroll.updated = true;
      }
      else
        s->mouse_wheel_scroll.value = 0;
    }

    void shutdown()
    {
      Allocator &a = *s->pads._allocator;

      a.deallocate(s->keyboard_buttons);
      a.deallocate(s->mouse_buttons);

      for (u32 i = 0; i < GLFW_JOYSTICK_LAST; i++) {
        if (!s->pads[i].name) continue;

        a.deallocate(s->pads[i].name);
        a.deallocate(s->pads[i].button_states);
      }

      MAKE_DELETE(a, InputSystem, s);

      glfwSetScrollCallback(w, NULL);
      w = NULL;
    }
  }

  namespace pad
  {
    bool active(u32 pad)
    {
      return (glfwJoystickPresent((i32)pad) == GL_TRUE &&
              s->pads[pad].current == PAD_CONNECTED);
    }

    const char *name(u32 pad)
    {
      return glfwGetJoystickName((i32)pad);
    }

    bool connected(u32 pad)
    {
      return (s->pads[pad].current  == PAD_CONNECTED &&
              s->pads[pad].previous == PAD_DISCONNECTED);
    }

    bool disconnected(u32 pad)
    {
      return (s->pads[pad].current  == PAD_DISCONNECTED &&
              s->pads[pad].previous == PAD_CONNECTED);
    }

    i32 num_buttons(u32 pad)
    {
      return s->pads[pad].num_buttons;
    }

    bool button(u32 pad, u32 key)
    {
      return s->pads[pad].button_states[key].current;
    }

    bool pressed(u32 pad, u32 key)
    {
      return s->pads[pad].button_states[key].current &&
            !s->pads[pad].button_states[key].previous;
    }

    bool released(u32 pad, u32 key)
    {
      return !s->pads[pad].button_states[key].current &&
              s->pads[pad].button_states[key].previous;
    }

    bool any_pressed(u32 pad)
    {
      for (i32 i = 0; i < s->pads[pad].num_buttons; i++)
        if (s->pads[pad].button_states[i].current &&
           !s->pads[pad].button_states[i].previous)
            return true;
      return false;
    }

    bool any_released(u32 pad)
    {
      for (i32 i = 0; i < s->pads[pad].num_buttons; i++)
        if (!s->pads[pad].button_states[i].current &&
             s->pads[pad].button_states[i].previous)
            return true;
      return false;
    }

    i32 num_axes(u32 pad)
    {
      i32 num_axes;
      glfwGetJoystickAxes(pad, &num_axes);
      return num_axes;
    }

    f32 axes(u32 pad, u32 i)
    {
      i32 num_axes;
      return *(glfwGetJoystickAxes(pad, &num_axes) + i);
    }
  }

  namespace mouse
  {
    glm::vec2 get_position()
    {
      f64 x, y;
      glfwGetCursorPos(w, &x, &y);
      return glm::vec2((f32)x, (f32)y);
    }

    void set_position(glm::vec2 &position)
    {
      glfwSetCursorPos(w, position.x, position.y);
    }

    bool button(u32 key)
    {
      return s->mouse_buttons[key].current;
    }

    bool pressed(u32 key)
    {
      return s->mouse_buttons[key].current &&
            !s->mouse_buttons[key].previous;
    }

    bool released(u32 key)
    {
      return !s->mouse_buttons[key].current &&
              s->mouse_buttons[key].previous;
    }

    bool any_pressed()
    {
      for (u32 i = 0; i < MOUSE_NUM_BUTTONS; i++)
        if (s->mouse_buttons[i].current && 
           !s->mouse_buttons[i].previous)
            return true;
      return false;
    }

    bool any_released()
    {
      for (u32 i = 0; i < MOUSE_NUM_BUTTONS; i++)
        if (!s->mouse_buttons[i].current && 
             s->mouse_buttons[i].previous)
            return true;
      return false;
    }

    i32 wheel_scroll()
    {
      return s->mouse_wheel_scroll.value;
    }
  }

  namespace keyboard
  {
    bool button(u32 key)
    {
      return s->keyboard_buttons[key].current;
    }

    bool pressed(u32 key)
    {
      return s->keyboard_buttons[key].current &&
            !s->keyboard_buttons[key].previous;
    }

    bool released(u32 key)
    {
      return !s->keyboard_buttons[key].current &&
              s->keyboard_buttons[key].previous;
    }

    bool any_pressed()
    {
      for (u32 i = 0; i < KEYBOARD_NUM_BUTTONS; i++)
        if (s->keyboard_buttons[i].current &&
           !s->keyboard_buttons[i].previous)
            return true;
      return false;
    }

    bool any_released()
    {
      for (u32 i = 0; i < KEYBOARD_NUM_BUTTONS; i++)
        if (!s->keyboard_buttons[i].current &&
             s->keyboard_buttons[i].previous)
            return true;
      return false;
    }

  }
}