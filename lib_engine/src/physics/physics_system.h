#pragma once

#include <runtime/types.h>
#include <runtime/memory_types.h>
#include <runtime/collection_types.h>
#include <data/actor.h>

#include "physics_types.h"

namespace pge
{
  namespace physics_system
  {
    // ---------------------------------------------------------------
    // Global system functions
    // ---------------------------------------------------------------

    void init     (u32 res_name, Allocator &a);
    i32  get_ppm  (void);
    void shutdown (void);

    const ActorTemplate    &get_actor_template(u32 name);
    const MaterialTemplate &get_material_template(u32 name);
    const ShapeTemplate    &get_shape_template(u32 name);

    // ---------------------------------------------------------------
    // Actor Manipulation
    // ---------------------------------------------------------------

    u64  create_actor  (PhysicsWorld &w, const ActorData &actor_data, u32 group = 0);
    void destroy_actor (PhysicsWorld &w, u64 actor);
    
    bool is_static     (PhysicsWorld &w, u64 actor);
    bool is_dynamic    (PhysicsWorld &w, u64 actor);
    bool is_physical   (PhysicsWorld &w, u64 actor);
    bool is_kinematic  (PhysicsWorld &w, u64 actor);
    void set_kinematic (PhysicsWorld &w, u64 actor, bool value);

    void get_velocity  (PhysicsWorld &w, u64 actor, glm::vec3 &v);
    void set_velocity  (PhysicsWorld &w, u64 actor, const glm::vec3 &v);

    void set_touched_callback   (PhysicsWorld &w, u64 actor, ContactCallback callback, const void *user_data);
    void set_untouched_callback (PhysicsWorld &w, u64 actor, ContactCallback callback, const void *user_data);
    void set_collision_filter   (PhysicsWorld &w, u64 actor, const char *filter);

    void add_impulse   (PhysicsWorld &w, u64 actor, const glm::vec3 &impulse);

    Pose &get_pose     (PhysicsWorld &w, u64 actor);

    // ---------------------------------------------------------------
    // Mover Manipulation
    // ---------------------------------------------------------------

    u64  create_mover    (PhysicsWorld &w, const MoverResource *r, u32 group = 0);
    void destroy_mover   (PhysicsWorld &w, u64 mover);

    void get_mover_position (PhysicsWorld &w, u64 mover, glm::vec3 &p);
    void teleport_mover     (PhysicsWorld &w, u64 mover, const glm::vec3 &p);
    void move_mover         (PhysicsWorld &w, u64 mover, const glm::vec3 &dp);
    void set_mover_collision_filter (PhysicsWorld &w, u64 mover, const char *filter);

    // ---------------------------------------------------------------
    // World manipulation & querying
    // ---------------------------------------------------------------

    void allow_simulations (PhysicsWorld &w, bool value);
    void update            (PhysicsWorld &w, f64 delta_time);
    void finalize_update   (PhysicsWorld &w);
    void set_gravity       (PhysicsWorld &w, const glm::vec3 &v);

    u64  create_raycast  (PhysicsWorld &w, RaycastCallback callback, bool closest, bool any, const char *filter);
    void cast_raycast    (PhysicsWorld &w, u64 raycast, const glm::vec3 &from, const glm::vec3 &to);
    void destroy_raycast (PhysicsWorld &w, u64 raycast);
  }
}