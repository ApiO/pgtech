#include "sample.h"

namespace app
{
  using namespace pge;

  namespace sample_font
  {
    u64 left, center, right;
    void init()
    {
      glm::vec3 pose = IDENTITY_TRANSLATION;
      glm::quat rot  = IDENTITY_ROTATION;

      pose.x = -3.f * global_screen.width / 8.f;
      pose.y = 3.f * global_screen.height / 8.f;
      rot = glm::angleAxis(0.f, glm::vec3(0.f, 0.f, 1.f));

      ///*
      left = world::spawn_text(global_game_world, global_font_name, ":)\nabcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ\n0123456789\n<²&é\"'ù(-è_çà)$ù*,;:!=\n>./§¨%~#{[|`\\^@]}¤\n\n(V)_(;,,;)_(V)",
                        TEXT_ALIGN_LEFT, pose, rot);
      //*/
      pose.x =  global_screen.width * (3.f / 8.f);
      pose.y = 200.f;
      rot = glm::angleAxis(glm::radians(-37.5f), glm::vec3(0.f, 0.f, 1.f));
      center = world::spawn_text(global_game_world, global_font_name, "text align\ncenter", TEXT_ALIGN_CENTER, pose, rot);

      pose.x = -250.f;
      pose.y = -150.f;
      rot = glm::angleAxis(glm::radians(-90.f), glm::vec3(0.f, 0.f, 1.f));
      right = world::spawn_text(global_game_world, global_font_name, "text align\nright", TEXT_ALIGN_RIGHT, pose, rot);
    }

    void shutdown()
    {
      world::despawn_text(global_game_world, left);
      world::despawn_text(global_game_world, center);
      world::despawn_text(global_game_world, right);
    }
  }
}