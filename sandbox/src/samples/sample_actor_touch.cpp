#include <runtime/array.h>
#include <runtime/assert.h>
#include <runtime/trace.h>
#include "sample.h"

namespace
{
  using namespace app;
  using namespace pge;

  u64 touch_log;

  f32 w2, h2;
  glm::vec3 t(IDENTITY_TRANSLATION);
  glm::quat q(glm::angleAxis(glm::radians(45.f), glm::vec3(0, 0, 1)));
  f32 angle = 0;
  const f32 MOVE_SPPED = 2 * 60;

  u64 circle, box;

  void touch_callback(const Array<ContactPoint> &contacts, const void *user_data)
  {
    ASSERT(array::size(contacts) == 1);

    char buf[512] = "\0";
    char tmp[64];

    sprintf(tmp, "TOUCHED: %llu - user data: %d", contacts[0].actor, *(i32*)user_data);
    strcat(buf, tmp);

    text::set_string(app::global_gui_world, touch_log, buf);
  }

  void untouch_callback(const Array<ContactPoint> &contacts, const void *user_data)
  {
    ASSERT(array::size(contacts) == 1);

    char buf[512] = "\0";
    char tmp[64];

    sprintf(tmp, "UNTOUCHED: %llu - user data: %d", contacts[0].actor, *(i32*)user_data);
    strcat(buf, tmp);

    text::set_string(app::global_gui_world, touch_log, buf);
  }

}

namespace app
{
  using namespace pge;

  i32 user_data_touch, user_data_untouch;

  namespace sample_actor_touch
  {
    void init()
    {
      //global_debug_physic = true;
      
      w2 = global_screen.w2;
      h2 = global_screen.h2;

      user_data_touch = 42;
      user_data_untouch = 24;

      circle = world::spawn_actor_circle(global_game_world, h2*0.25f,
                                         true, true, false, true,
                                         "default", "solid",
                                         glm::vec3(-w2, h2, 0.f), q);

      box = world::spawn_actor_box(global_game_world, global_screen.width*.15f, global_screen.height*.15f,
                                   false, false, false, false,
                                   "default", "solid", t, q);

      actor::set_touched_callback(global_game_world, box, touch_callback, &user_data_touch);
      actor::set_untouched_callback(global_game_world, box, untouch_callback, &user_data_untouch);

      touch_log = world::spawn_text(global_gui_world, global_font_name, "-LOG-", TEXT_ALIGN_LEFT, glm::vec3(-w2 + 10, h2 - 10, 0.f), glm::quat(IDENTITY_ROTATION));

    }

    void update(f64 dt)
    {
      angle += (f32)(dt* MOVE_SPPED);
      if (angle > 360.f) angle-= 360;
      
      glm::vec3 position(0, sin(glm::radians(angle))*h2, 0);
      actor::set_local_position(global_game_world, circle, position);
    }

    void shutdown()
    {
      world::despawn_actor(global_game_world, circle);
      world::despawn_actor(global_game_world, box);
      world::despawn_text(global_gui_world, touch_log);
    }
  }
}