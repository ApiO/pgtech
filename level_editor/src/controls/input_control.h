#pragma once

#include <glm/glm.hpp>
#include <runtime/types.h>
#include <bases/form.h>
#include <bases/focusable.h>
#include <utils/timer.h>

#include <runtime/trace.h>

namespace app
{
  namespace controls
  {
    using namespace pge;
    using namespace bases;
    using namespace utils;

    enum InputFormat
    {
      INPUT_FORMAT_TEXT = 0,
      INPUT_FORMAT_INTEGER,
      INPUT_FORMAT_UNSIGNED_INTEGER,
      INPUT_FORMAT_LONG,
      INPUT_FORMAT_UNSIGNED_LONG,
      INPUT_FORMAT_FLOAT,
      INPUT_FORMAT_DOUBLE,
      INPUT_FORMAT_PATH,
      INPUT_FORMAT_EXADECIMAL
    };

    typedef void(*OnValidate) (void*);

    class InputControl : public Form, public Focusable
    {
    public:
      InputControl();
      ~InputControl();
      void update(f64 delta_time);
      void draw(void);
      void initialize(const char *label, const char *value, i32 x, i32 y, i32 label_width, i32 input_width, InputFormat format);
      void set_position(i32 x, i32 y);
      void set_label(const char *label);
      void set_value(const char *value, bool edited = true);
      void set_label_width(i32 value);
      void set_input_width(i32 value);
      void set_min(void *value);
      void set_max(void *value);
      void clear(void);
      const char *get_value(void);
      void set_validation_callback(OnValidate func, void *user_data);
    private:
      struct AABB {
        glm::vec2 min, max;
      };
      AABB  aabb;
      void *user_data;
      char *input_string;
      i32   label_width;
      i32   input_width;
      char  label[128];
      char  value[128];
      char  value_backup[128];
      Timer last_char_deletion;
      bool  edited;
      void *min_value;
      void *max_value;
      void  propagate_validation(void);
      OnValidate  on_validate;
      InputFormat format;
      void on_focus_callback(void);
      void on_focus_lost_callback(void);
    };
  }

  namespace controls
  {
    inline InputControl::InputControl() : on_validate(NULL), user_data(NULL),
      min_value(NULL), max_value(NULL),
      edited(false)
    {
      utils::timer::start(last_char_deletion);
    }

    inline void InputControl::set_label_width(i32 value)
    {
      label_width = value;
    }

    inline void InputControl::set_input_width(i32 value)
    {
      input_width = value;
    }

    inline void InputControl::clear(void)
    {
      if (!strlen(value)) return;
      edited = true;
      value[0] = '\0';
    }

    inline const char *InputControl::get_value(void)
    {
      return value;
    }


    inline void InputControl::set_validation_callback(OnValidate func, void *ud)
    {
      on_validate = func;
      trigger_focus_lost = func != NULL;
      user_data = ud;
    }

    inline void InputControl::propagate_validation(void)
    {
      if (!edited) return;

      edited = false;

      if (on_validate)
        on_validate(user_data);
    }

    inline void InputControl::on_focus_callback(void){}

    inline void InputControl::on_focus_lost_callback(void)
    {
      propagate_validation();
    }
    
    inline void InputControl::set_value(const char *v, bool edited)
    {
      value[0] = '\0';
      edited = edited;
      strcat(value, v);
    }

    inline void InputControl::set_label(const char *l)
    {
      label[0] = '\0';
      strcat(label, l);
    }
  }
}