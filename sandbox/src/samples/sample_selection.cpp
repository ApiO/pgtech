#include <runtime/array.h>
#include <runtime/idlut.h>
#include "sample.h"

namespace app
{
  namespace
  {
    using namespace pge;

    struct Selectable {
      enum {
        TYPE_NONE,
        TYPE_UNIT,
        TYPE_SPRITE,
        TYPE_ACTOR,
        TYPE_TEXT
      } type;
      u64 id;
    };

    struct FindResult : Selectable {
      Box box;
    };

    void get_selection(u64 world, u64 camera, const glm::vec2 &mouse_pos, const IdLookupTable<Selectable> &selectables, FindResult &result)
    {
      Box b;
      const IdLookupTable<Selectable>::Entry *e, *end = idlut::end(selectables);
      glm::vec3 from, dir;

      camera::screen_to_world(world, camera, glm::vec3(mouse_pos.x, mouse_pos.y, 1), dir);
      camera::screen_to_world(world, camera, glm::vec3(mouse_pos.x, mouse_pos.y, 0), from);
      dir = glm::normalize(dir - from);

      for (e = idlut::begin(selectables); e < end; e++) {
        switch (e->value.type) {
          case Selectable::TYPE_SPRITE:
            sprite::box(world, e->value.id, b);
            break;
          case Selectable::TYPE_UNIT:
            unit::box(world, e->value.id, b);
            break;
          default:
            XERROR("Not implemented yet...");
        }
        f32 d = math::ray_box_intersection(from, dir, b);
        printf("%f\n", d);
        if (d > 0) {
          result.id   = e->value.id;
          result.type = e->value.type;
          result.box  = b;
          return;
        } 
      }
      result.type = Selectable::TYPE_NONE;
    }
  }

  namespace sample_selection
  {
    using namespace pge;

    IdLookupTable<Selectable> *selectables = NULL;
    FindResult selected;

    void init()
    {
      Allocator &a = memory_globals::default_allocator();
      selectables = MAKE_NEW(a, IdLookupTable<Selectable>, a);

      glm::vec3 p(0, 0, 2000);
      camera_free::initialize(p);
      world::physics_simulations(global_game_world, false);

      Selectable e;
      e.id = world::spawn_unit(global_game_world, "units/dragon/dragon", glm::vec3(0,-100,-100), glm::quat(1,0,0,0));
      e.type = Selectable::TYPE_UNIT;
      idlut::add(*selectables, e);

      e.id = world::spawn_unit(global_game_world, "units/myball", glm::vec3(-100,-200,0), glm::quat(1,0,0,0));
      e.type = Selectable::TYPE_UNIT;
      idlut::add(*selectables, e);

      e.id = world::spawn_sprite(global_game_world, "units/spineboy/sprites/eyes", glm::vec3(0,-200,0), glm::quat(1,0,0,0));
      e.type = Selectable::TYPE_SPRITE;
      idlut::add(*selectables, e);

      e.id = world::spawn_sprite(global_game_world, "units/spineboy/sprites/head", glm::vec3(100,-200,0), glm::quat(1,0,0,0));
      e.type = Selectable::TYPE_SPRITE;
      idlut::add(*selectables, e);

      //world::spawn_sprite(global_game_world, "units/dragon/sprites/R_wing", glm::vec3(100,-300,0), glm::quat(1,0,0,0));
    }

    void update(f64 dt)
    {
      camera_free::update(dt);

      if (!mouse::pressed(MOUSE_KEY_1))
        return;

      get_selection(global_game_world, global_game_camera.get_id(), mouse::get_position(), *selectables, selected);
    }

    void render()
    {
      if (selected.type == Selectable::TYPE_NONE)
        return;

      glm::vec3 v[4];
      for (i32 i = 0; i < 4; i++)
        v[i].z = (selected.box.min.z + selected.box.max.z)/2;

      v[0].x = selected.box.min.x;
      v[0].y = selected.box.min.y;

      v[1].x = selected.box.max.x;
      v[1].y = selected.box.min.y;

      v[2].x = selected.box.max.x;
      v[2].y = selected.box.max.y;

      v[3].x = selected.box.min.x;
      v[3].y = selected.box.max.y;

      application::render_polygon(v, 4, selected.type == Selectable::TYPE_UNIT ? 
                                  Color(255,0,220,255) : Color(255,106,0,255), 
                                  global_game_world, global_game_camera.get_id(), global_viewport);
    }

    void shutdown()
    {
      Allocator &a = memory_globals::default_allocator();

      const IdLookupTable<Selectable>::Entry *e, *end = idlut::end(*selectables);

      for (e = idlut::begin(*selectables); e < end; e++) {
        switch (e->value.type) {
          case Selectable::TYPE_SPRITE:
            world::despawn_sprite(global_game_world, e->value.id);
            break;
          case Selectable::TYPE_UNIT:
            world::despawn_unit(global_game_world, e->value.id);
            break;
          default:
            XERROR("Not implemented yet...");
        }
      }

      world::physics_simulations(global_game_world, true);
      camera_free::reset_ortho();
      MAKE_DELETE(a, IdLookupTable<Selectable>, selectables);
    }
  }
}