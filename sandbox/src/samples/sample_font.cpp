#include "sample.h"

namespace app
{
  using namespace pge;

  namespace sample_font
  {
    u64 left, center, right, scale, color;
    void init()
    {
      glm::vec3 p = IDENTITY_TRANSLATION;
      glm::quat r = IDENTITY_ROTATION;

      p.x = -3.f * global_screen.width / 8.f;
      p.y = 3.f * global_screen.height / 8.f;
      r = glm::angleAxis(0.f, glm::vec3(0.f, 0.f, 1.f));

      left = world::spawn_text(global_game_world, global_font_name, ":)\nabcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ\n0123456789\n<²&é\"'ù(-è_çà)$ù*,;:!=\n>./§¨%~#{[|`\\^@]}¤\n\n(V)_(;,,;)_(V)",
                        TEXT_ALIGN_LEFT, p, r);
      
      p.x =  global_screen.width * (3.f / 8.f);
      p.y = 200.f;
      r = glm::angleAxis(glm::radians(-37.5f), glm::vec3(0.f, 0.f, 1.f));
      center = world::spawn_text(global_game_world, global_font_name, "Text align\ncenter", TEXT_ALIGN_CENTER, p, r);

      p.x = -250.f;
      p.y = -150.f;
      r = glm::angleAxis(glm::radians(-90.f), glm::vec3(0.f, 0.f, 1.f));
      right = world::spawn_text(global_game_world, global_font_name, "Text align\nright", TEXT_ALIGN_RIGHT, p, r);


      p.x *= -1;
      color = world::spawn_text(global_game_world, global_font_name, "***\nRed text\n***", TEXT_ALIGN_CENTER, p, IDENTITY_ROTATION);
      text::set_color(global_game_world, color, Color(255.f, 0.f, 0.f, 255.f));

      p = IDENTITY_TRANSLATION;
      p.y = global_screen.h2*0.75f;
      r = glm::angleAxis(glm::radians(-90.f), glm::vec3(0.f, 0.f, 1.f));
      scale = world::spawn_text(global_game_world, global_font_name, "***\nScale x5\n***", TEXT_ALIGN_CENTER, p, IDENTITY_ROTATION);
      text::set_scale(global_game_world, scale, 5.f);
    }

    void shutdown()
    {
      world::despawn_text(global_game_world, left);
      world::despawn_text(global_game_world, center);
      world::despawn_text(global_game_world, right);
      world::despawn_text(global_game_world, scale);
      world::despawn_text(global_game_world, color);
    }
  }
}