#pragma once

#include <glm/glm.hpp>

#include <runtime/types.h>
#include "physics_types.h"

namespace pge
{
  typedef void(*WrapperContactCallback)(const glm::vec2 &position, f32 distance, const glm::vec2 &normal, const Body *body, void *data);

  enum BodyType
  {
    BODY_TYPE_STATIC = 0,
    BODY_TYPE_KINEMATIC,
    BODY_TYPE_DYNAMIC
  };

  namespace physics
  {
    const u16 IS_ALL = 0xFFFF;
    const u16 COLLIDES_WITH_ALL   = 0xFFFF;
    const i32 VELOCITY_ITERATIONS = 8;
    const i32 POSITION_ITERATIONS = 2;

    void initialize (i32 velocity_iterations, i32 position_iterations);

    // SPACE

    Space *create_space  (void);
    Space *create_space  (f32 gravity_x, f32 gravity_y);
    void   destroy_space (Space *space);
    void   update_space  (Space *space, f32 delta_time);
    void   set_gravity   (Space *space, f32 x, f32 y);

    // BODY

    Body *create_body       (Space *space, f32 offset_x, f32 offset_y, BodyType type);
    void  destroy_body      (Body *body);
    void  set_body_type     (Body *body, BodyType type);
    void  get_body_position (const Body *body, glm::vec2 &v);
    f32   get_body_rotation (const Body *body);
    void  set_body_position (Body *body, const glm::vec2 &pos);
    void  set_body_rotation (Body *body, f32 rotation);
    void  set_awake         (Body *body, bool v);
    bool  is_sleeping       (const Body *body);
    void  get_velocity      (const Body *body, glm::vec2 &v);
    void  set_velocity      (Body *body, const glm::vec2 &v);
    void  enable_gravity    (Body *body);
    void  disable_gravity   (Body *body);
    void  set_body_filter   (Body *body, u16 is, u16 collides_with);

    void  apply_impulse     (Body *body, const glm::vec2 &impulse);

    // SHAPE
    
    Shape *create_circle      (f32 radius, const glm::vec2 &offset);
    Shape *create_chain       (const glm::vec2 *vertices, i32 num_verts);
    Shape *create_polygon     (const glm::vec2 *vertices, i32 num_verts);
    void   destroy_shape      (Shape *shape);
    void   get_shape_position (const Shape *shape, glm::vec2 &p);
    void   set_shape_position (Shape *shape, const glm::vec2 &pos);
    f32    get_circle_radius  (const Shape *circle);
    i32    get_num_vertices   (const Shape *shape);
    const  glm::vec2 *get_vertices (const Shape *shape);

    // FIXTURE

    Fixture *create_fixture  (Body *body, Shape *shape, f32 density, f32 friction, f32 restitution);
    Fixture *create_fixture  (Body *body, Shape *shape, Fixture *other);
    void     destroy_fixture (Body *body, Fixture *fixture);

    void set_fixture_position (Fixture *fixture, const glm::vec2 &pos);
    void get_fixture_position (const Fixture *fixture, glm::vec2 &p);
    void get_fixture_world_position (const Fixture *fixture, glm::vec2 &p);

    void set_density     (Fixture *fixture, f32 value);
    void set_friction    (Fixture *fixture, f32 value);
    void set_restitution (Fixture *fixture, f32 value);

    void set_filter  (Fixture *fixture, u16 is, u16 collides_with);
    void set_group   (Fixture *fixture, u32 group);
    void set_trigger (Fixture *fixture, bool value);
    
    void get_body_contacts(Body *body, WrapperContactCallback callback, void *data);

    // query
    void raycast(Space *space, const glm::vec2 &from, const glm::vec2 &to,
                 WrapperContactCallback callback,
                 bool closest, bool any, u16 is, u16 collides_with, void *data);
  }
}