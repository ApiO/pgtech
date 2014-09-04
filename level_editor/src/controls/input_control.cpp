#include <string>

#include <engine/pge.h>
#include <runtime/assert.h>
#include <runtime/memory.h>

#include <application_types.h>
#include"input_control.h"

using namespace pge;
using namespace app::bases;
using namespace app::utils::timer;

namespace
{
  using namespace app;

  enum InputState
  {
    INPUT_STATE_NONE = 0,
    INPUT_STATE_SHIFT,
    INPUT_STATE_ALT
  };

  static bool handle_input(char *input, KeyboardKey key, const char *value, const char *shift_value, const char *alt_value, InputState sate, bool &edited)
  {
    if (keyboard::pressed(key)){
      if (strlen(input) == _MAX_PATH) return false;
      switch (sate)
      {
      case INPUT_STATE_NONE:
        if (!value) return false;
        strcat(input, value);
        break;
      case INPUT_STATE_SHIFT:
        if (!shift_value) return false;
        strcat(input, shift_value);
        break;
      case INPUT_STATE_ALT:
        if (!alt_value) return false;
        strcat(input, alt_value);
        break;
      }
      edited = true;
    }
    return true;
  }

  inline bool handle_chars(char *input, InputState sate, bool &edited)
  {
    if (strlen(input) == _MAX_PATH) return false;

    if (!handle_input(input, KEYBOARD_KEY_A, "a", "A", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_B, "b", "B", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_C, "c", "C", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_D, "d", "D", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_E, "e", "E", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_F, "f", "F", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_G, "g", "G", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_H, "h", "H", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_I, "i", "I", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_J, "j", "J", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_K, "k", "K", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_L, "l", "L", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_M, "m", "M", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_N, "n", "N", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_O, "o", "O", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_P, "p", "P", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_Q, "q", "Q", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_R, "r", "R", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_S, "s", "S", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_T, "t", "T", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_U, "u", "U", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_V, "v", "V", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_W, "w", "W", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_X, "x", "X", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_Y, "y", "Y", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_Z, "z", "Z", NULL, sate, edited)) return false;

    return true;
  }

  inline bool handle_numbers(char *input, InputState sate, bool &edited)
  {
    if (strlen(input) == _MAX_PATH) return false;

    if (!handle_input(input, KEYBOARD_KEY_0, NULL, "0", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_1, NULL, "1", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_2, NULL, "2", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_3, NULL, "3", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_4, NULL, "4", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_5, NULL, "5", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_6, NULL, "6", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_7, NULL, "7", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_8, NULL, "8", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_9, NULL, "9", NULL, sate, edited)) return false;

    if (!handle_input(input, KEYBOARD_KEY_KP_0, "0", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_1, "1", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_2, "2", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_3, "3", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_4, "4", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_5, "5", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_6, "6", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_7, "7", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_8, "8", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_9, "9", NULL, NULL, sate, edited)) return false;

    return true;
  }

  inline bool handle_special_chars(char *input, InputState sate, bool &edited)
  {
    if (strlen(input) == _MAX_PATH) return false;

    if (!handle_input(input, KEYBOARD_KEY_KP_DIVIDE, "/", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_MULTIPLY, "*", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_SUBTRACT, "-", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_ADD, "+", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_DECIMAL, ".", NULL, NULL, sate, edited)) return false;

    if (!handle_input(input, KEYBOARD_KEY_1, "&", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_2, "é", NULL, "~", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_3, "\"", NULL, "#", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_4, "'", NULL, "{", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_5, "(", NULL, "[", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_6, "-", NULL, "|", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_7, "è", NULL, "`", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_8, "_", NULL, "\\", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_9, "ç", NULL, "^", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_0, "à", NULL, "@", sate, edited)) return false;

    if (!handle_input(input, KEYBOARD_KEY_LEFT_BRACKET, ")", "°", "]", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_EQUAL, "=", "+", "}", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_RIGHT_BRACKET, "^", "¨", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_SEMICOLON, "$", "£", "¤", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_GRAVE_ACCENT, "ù", "%", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_BACKSLASH, "*", "µ", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_COMMA, ",", "?", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_PERIOD, ";", ".", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_SLASH, ":", "/", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_WORLD_1, "!", "§", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_WORLD_2, "<", ">", NULL, sate, edited)) return false;

    return true;
  }

  // spe

  inline bool handle_exa_chars(char *input, InputState sate, bool &edited)
  {
    if (strlen(input) == _MAX_PATH) return false;

    if (!handle_input(input, KEYBOARD_KEY_A, "a", "A", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_A, "b", "B", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_A, "c", "C", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_A, "d", "D", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_A, "e", "E", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_A, "f", "F", NULL, sate, edited)) return false;

    return true;
  }

  inline bool handle_path_special_chars(char *input, InputState sate, bool &edited)
  {
    if (strlen(input) == _MAX_PATH) return false;

    // unsecure impl: see, http://msdn.microsoft.com/en-us/library/aa365247%28VS.85%29

    if (!handle_input(input, KEYBOARD_KEY_KP_SUBTRACT, "-", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_ADD, "+", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_DECIMAL, ".", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_KP_DIVIDE, "/", NULL, NULL, sate, edited)) return false;

    if (!handle_input(input, KEYBOARD_KEY_1, "&", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_2, "é", NULL, "~", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_3, NULL, NULL, "#", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_4, "'", NULL, "{", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_5, "(", NULL, "[", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_6, "-", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_7, "è", NULL, "`", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_8, "_", NULL, "\\", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_9, "ç", NULL, "^", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_0, "à", NULL, "@", sate, edited)) return false;

    if (!handle_input(input, KEYBOARD_KEY_LEFT_BRACKET, ")", "°", "]", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_EQUAL, "=", "+", "}", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_RIGHT_BRACKET, "^", "¨", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_SEMICOLON, "$", "£", "¤", sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_GRAVE_ACCENT, "ù", "%", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_BACKSLASH, NULL, "µ", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_COMMA, ",", NULL, NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_PERIOD, ";", ".", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_WORLD_1, "!", "§", NULL, sate, edited)) return false;
    if (!handle_input(input, KEYBOARD_KEY_SLASH, NULL, "/", NULL, sate, edited)) return false;

    return true;
  }
}

namespace app
{
  namespace controls
  {
    const i32 TEXT_MARING_TOP = 16;

    void InputControl::update(f64 delta_time)
    {
      (void)delta_time;

      if (!BOOL(_enable)) return;


      // handles focus
      if (mouse::pressed(MOUSE_KEY_1)){
        glm::vec2 mouse_position = mouse::get_position();
        if (!(mouse_position.x < aabb.min.x || mouse_position.y < aabb.min.y ||
          mouse_position.x > aabb.max.x || mouse_position.y > aabb.max.y))
          set_focus(true);
      }

      // handles user's input according to the current format 
      if (!is_focused()) return;

      // erase last char
      if (keyboard::button(KEYBOARD_KEY_BACKSPACE)){
        if (!strlen(value)) return;

        if (get_elapsed_time_in_sec(last_char_deletion) < .06f) return;

        edited = true;
        value[strlen(value) - 1] = '\0';
        start(last_char_deletion);
        return;
      }

      if (keyboard::any_pressed()) {

        if (keyboard::pressed(KEYBOARD_KEY_DELETE)){
          if (!strlen(value)) return;
          edited = true;
          clear();

          switch (format)
          {
          case INPUT_FORMAT_INTEGER:
          case INPUT_FORMAT_LONG:
            sprintf(value, "%d", min_value ? *(i32*)min_value : 0);
            break;
          case INPUT_FORMAT_UNSIGNED_INTEGER:
          case INPUT_FORMAT_UNSIGNED_LONG:
            sprintf(value, "%u", min_value ? *(u32*)min_value : 0u);
            break;
          case INPUT_FORMAT_FLOAT:
          case INPUT_FORMAT_DOUBLE:
            sprintf(value, "%.f", min_value ? *(f32*)min_value : 0.f);
            break;
          case INPUT_FORMAT_PATH:
          case INPUT_FORMAT_TEXT:
            value[0] = '\0';
            break;
          }
          return;
        }

        // VALIDATE
        if ((keyboard::pressed(KEYBOARD_KEY_ENTER) || keyboard::pressed(KEYBOARD_KEY_KP_ENTER)))
          propagate_validation();

        // handles inputs
        InputState sate = INPUT_STATE_NONE;
        if (keyboard::button(KEYBOARD_KEY_LEFT_SHIFT) || keyboard::button(KEYBOARD_KEY_RIGHT_SHIFT))
          sate = INPUT_STATE_SHIFT;
        if (keyboard::button(KEYBOARD_KEY_RIGHT_ALT))
          sate = INPUT_STATE_ALT;

        bool next;

        switch (format)
        {
        case INPUT_FORMAT_PATH:
          next = handle_chars(value, sate, edited);
          if (next) next = handle_numbers(value, sate, edited);
          if (next) handle_path_special_chars(value, sate, edited);
          break;
        case INPUT_FORMAT_FLOAT:
        case INPUT_FORMAT_DOUBLE:
          next = handle_numbers(value, sate, edited);
          if (next && keyboard::pressed(KEYBOARD_KEY_KP_DECIMAL) && strchr(value, '.')){
            if (strlen(value) < _MAX_PATH) strcat(value, ".");
            else next = false;
          }
          if (next && keyboard::pressed(KEYBOARD_KEY_KP_SUBTRACT)){
            char tmp[128];
            if (strchr(value, '-')){
              sprintf(tmp, "%s", value + 1);
              strcpy(value, tmp);
            }
            else{
              sprintf(tmp, "-%s", value);
              strcpy(value, tmp);
            }
          }
          break;
        case INPUT_FORMAT_TEXT:
          next = handle_chars(value, sate, edited);
          if (next) next = handle_numbers(value, sate, edited);
          if (next) handle_special_chars(value, sate, edited);
          break;
        case INPUT_FORMAT_INTEGER:
        case INPUT_FORMAT_UNSIGNED_INTEGER:
        case INPUT_FORMAT_LONG:
        case INPUT_FORMAT_UNSIGNED_LONG:
          next = handle_numbers(value, sate, edited);
          if (format != INPUT_FORMAT_UNSIGNED_INTEGER && format != INPUT_FORMAT_UNSIGNED_LONG &&
            next && keyboard::pressed(KEYBOARD_KEY_KP_SUBTRACT)){
            char tmp[128];
            if (strchr(value, '-')){
              sprintf(tmp, "%s", value + 1);
              strcpy(value, tmp);
            }
            else{
              sprintf(tmp, "-%s", value);
              strcpy(value, tmp);
            }
          }

          break;
        case INPUT_FORMAT_EXADECIMAL:
          next = handle_exa_chars(value, sate, edited);
          if (next) handle_numbers(value, sate, edited);
          break;
        default:
          XERROR("Input format not handled: %d", format);
        }
      }

    }
    
    void InputControl::draw(void)
    {
      imguiDrawText(x, y - TEXT_MARING_TOP, imguiTextAlign::IMGUI_ALIGN_LEFT, label, is_enable() ? WHITE_TEXT : DISABLE_GRAY_COLOR);
      imguiDrawRect((f32)(x + label_width), (f32)(y - INPUT_HEIGHT), (f32)input_width, (f32)INPUT_HEIGHT, is_enable() ? (is_focused() ? SELECT_COLOR : WHITE_TEXT) : DISABLE_GRAY_COLOR);
      imguiDrawText(x + label_width + 6, y - TEXT_MARING_TOP, imguiTextAlign::IMGUI_ALIGN_LEFT, value, is_enable() ? (is_focused() ? WHITE_TEXT : BLACK_TEXT) : GRAY_TEXT);
    }

    void InputControl::initialize(const char *_label, const char *_value, pge::i32 _x, pge::i32 _y, pge::i32 _lw, pge::i32 _iw, InputFormat _f)
    {
      set_label(_label);
      set_value(_value);

      x = _x;
      y = _y;
      label_width = _lw;
      input_width = _iw;
      format = _f;

      aabb.min.x = (f32)(x + label_width);
      aabb.min.y = screen.height - (f32)y;
      aabb.max.x = aabb.min.x + input_width;
      aabb.max.y = aabb.min.y + INPUT_HEIGHT;

      height = INPUT_HEIGHT;
      width = label_width + input_width;
    }

    void InputControl::set_position(pge::i32 _x, pge::i32 _y)
    {
      x = _x;
      y = _y;

      aabb.min.x = (f32)(x + label_width);
      aabb.min.y = screen.height - (f32)y;
      aabb.max.x = aabb.min.x + input_width;
      aabb.max.y = aabb.min.y + INPUT_HEIGHT;
    }

    void InputControl::set_min(void *v)
    {
      u32 size;
      switch (format)
      {
      case INPUT_FORMAT_INTEGER:
      case INPUT_FORMAT_UNSIGNED_INTEGER:
      case INPUT_FORMAT_FLOAT:
        size = 4u;
        break;
      case INPUT_FORMAT_LONG:
      case INPUT_FORMAT_UNSIGNED_LONG:
      case INPUT_FORMAT_DOUBLE:
        size = 8u;
        break;
      default: return;
      }
      Allocator &a = memory_globals::default_allocator();
      min_value = a.allocate(size);
      memcpy(min_value, v, size);
    }

    void InputControl::set_max(void *v)
    {
      u32 size;
      switch (format)
      {
      case INPUT_FORMAT_INTEGER:
      case INPUT_FORMAT_UNSIGNED_INTEGER:
      case INPUT_FORMAT_FLOAT:
        size = 4u;
        break;
      case INPUT_FORMAT_LONG:
      case INPUT_FORMAT_UNSIGNED_LONG:
      case INPUT_FORMAT_DOUBLE:
        size = 8u;
        break;
      default: return;
      }
      max_value = memory_globals::default_allocator().allocate(size);
      memcpy(max_value, v, size);
    }

    InputControl::~InputControl()
    {
      if (min_value)
        memory_globals::default_allocator().deallocate(min_value);
      if (max_value)
        memory_globals::default_allocator().deallocate(max_value);
    }
  }
}