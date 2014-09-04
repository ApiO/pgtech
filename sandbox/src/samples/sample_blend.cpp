#include <runtime/array.h>
#include "sample.h"

namespace
{
  bool intersect_plane(const glm::vec3 &n, const glm::vec3 &p0, const glm::vec3& l0, glm::vec3 &l, float &d)
  {
    glm::vec3 p0l0;
    // assuming vectors are all normalized
    float denom = glm::dot(n, l);
    if (denom > 1e-6) {
      p0l0 = p0 - l0;
      d = glm::dot(p0l0, n) / denom;
      return (d >= 0);
    }
    return false;
  }
}

namespace app
{
  namespace sample_blend
  {
    using namespace pge;

    u64 background;

    u64 gn, gm, gs, go;
    u64 tn, tm, ts, to;
    
    const glm::quat q(1, 0, 0, 0);
    const f32 pad_h = 212.5f;

    char *description = "Move mouse to see transforms.";

    static void spawn_stuff(u64 &sprite, u64 &text, const char *mode, const char *label, glm::vec3 position)
    {
      sprite = world::spawn_sprite(global_game_world, mode, position, q);
      position.y += pad_h;
      text = world::spawn_text(global_game_world, global_font_name, label, TEXT_ALIGN_LEFT, position, q);
    }


    void init()
    {
      global_sample_desciption = description;

      background = world::spawn_sprite(global_game_world, "sprites/bg", glm::vec3(0, 0, -1), q);

      spawn_stuff(gn, tn, "sprites/gradient_normal", "normal", glm::vec3(-850, 0, 0));
      spawn_stuff(gm, tm, "sprites/gradient_multiply", "multiply", glm::vec3(-425, 0, 0));
      spawn_stuff(gs, ts, "sprites/gradient_screen", "screen", glm::vec3(0, 0, 0));
      spawn_stuff(go, to, "sprites/gradient_overlay", "overlay", glm::vec3(425, 0, 0));
    }

    void update(f64 dt)
    {
      (void)dt;

      glm::vec2 mouse_pos = mouse::get_position();
      glm::vec3 p, pn, pf, dir;
      f32 l;

      camera::screen_to_world(global_game_world, global_game_camera.get_id(), glm::vec3(mouse_pos.x, mouse_pos.y, 0), pn);
      camera::screen_to_world(global_game_world, global_game_camera.get_id(), glm::vec3(mouse_pos.x, mouse_pos.y, 1), pf);

      dir = glm::normalize(pf - pn);

      if (intersect_plane(glm::vec3(0, 0, -1), glm::vec3(0, 0, 0), pn, dir, l))
      {
        pn = pn + dir*l;
        sprite::set_local_position(global_game_world, gs, pn);
        p = pn;
        p.y += pad_h;
        text::set_local_position(global_game_world, ts, p);

        pn.x -= 425;
        sprite::set_local_position(global_game_world, gm, pn);
        p = pn;
        p.y += pad_h;
        text::set_local_position(global_game_world, tm, p);

        pn.x -= 425;
        sprite::set_local_position(global_game_world, gn, pn);
        p = pn;
        p.y += pad_h;
        text::set_local_position(global_game_world, tn, p);

        pn.x += 425 * 3;
        sprite::set_local_position(global_game_world, go, pn);
        p = pn;
        p.y += pad_h;
        text::set_local_position(global_game_world, to, p);

      }
    }

    void shutdown()
    {
      world::despawn_sprite(global_game_world, background);

      world::despawn_sprite(global_game_world, gn);
      world::despawn_sprite(global_game_world, gm);
      world::despawn_sprite(global_game_world, gs);
      world::despawn_sprite(global_game_world, go);

      world::despawn_text(global_game_world, tn);
      world::despawn_text(global_game_world, tm);
      world::despawn_text(global_game_world, ts);
      world::despawn_text(global_game_world, to);
    }
  }
}