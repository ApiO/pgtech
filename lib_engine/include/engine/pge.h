#pragma once

#include "pge_types.h"
#include <runtime/memory_types.h>
#include <runtime/collection_types.h>

namespace pge
{
  namespace application
  {
    // Initializes the application.

    void init(Init init, Update update, Render render, Shutdown shutdown,
      const char *data_dir, Allocator &a, bool create_window = true);

    void init(Init init, Update update, Render render, Shutdown shutdown, Synchro cb_synchro,
      RenderInit render_init, RenderBegin render_begin, RenderEnd render_end, RenderShutdown render_shutdown,
      const char *data_dir, Allocator &a, bool create_window = true);

    // Creates a new game world.
    u64 create_world();

    // Renders the world using the camera into the viewport.
    void render_world(u64 world, u64 camera, u64 viewport);

    // Destroys the specified world.
    void destroy_world(u64 world);

    // Returns the array containing all the game worlds
    void get_worlds(Array<u64> &worlds);

    // Creates a resource package for the specified name.
    u64 resource_package(const char *name);

    // Frees a resource package previously allocated with 'resource_package()'
    void release_resource_package(u64 package);

    // Creates a viewport to be used when rendering.
    u64 create_viewport(u32 x, u32 y, u32 width, u32 height);

    void set_viewport(u64 viewport, u32 x, u32 y, u32 width, u32 height);

    // Destroys the 'viewport'.
    void destroy_viewport(u64 viewport);

    // Updates the application (do not use in callbacks).
    void update(void);

    // Returns true if the application is closing
    bool should_quit(void);

    // Quits the application
    void quit(void);
    
    // Shutdowns the application (do not use in callbacks).
    void shutdown(void);

    void render_line(const glm::vec3 &a, const glm::vec3 &b, const Color &color, u64 world, u64 camera, u64 viewport);

    void render_polygon(const glm::vec3 *vertices, u32 num_vertices, const Color &color, u64 world, u64 camera, u64 viewport);

    void render_circle(const glm::vec3 &center, f32 radius, const Color &color, u64 world, u64 camera, u64 viewport, bool surface = false);

    // Renders the frustum of a camera.
    void render_frustum(u64 camera, u64 world, u64 render_camera, u64 viewport);

    void set_autoload(bool enabled);

    void show_culling_debug(bool value);
  }

  namespace world
  {
    // Updates the world animations & scene.
    void update(u64 world, f64 delta_time);

    // Updates the 'world' animations only.
    void update_anims(u64 world, f64 delta_time);

    // Updates the 'world' physics and scene graphs only.
    void update_scene(u64 world, f64 delta_time);

    // Returns the last delta time used for the 'world' update.
    f64 delta_time(u64 world);

    // Returns the time ellapsed since the 'world' creation.
    f64 total_time(u64 world);

    // Spawns a new 'name' unit at the specified 'translation' and 'rotation'.
    u64 spawn_unit(u64 world, const char *name, const glm::vec3 &translation, const glm::quat &rotation, const glm::vec3 &scale = glm::vec3(1.f));

    // Spawns a new camera at the specified 'translation' and 'rotation'.
    u64 spawn_camera(u64 world, f32 aspect, const glm::vec3 &position, const glm::quat &rotation);

    // Spawns a new 'name' sprite at the specified 'translation' and 'rotation'.
    u64 spawn_sprite(u64 world, const char *name, const glm::vec3 &position, const glm::quat &rotation, const glm::vec3 &scale = glm::vec3(1.f));

    // Spawns a new text with the specified 'font', 'text' and 'align' at the specified 'translation' and 'rotation'.
    u64 spawn_text(u64 world, const char *font, const char *text, TextAlign align, const glm::vec3 &position, const glm::quat &rotation, f32 scale = 1.f, const Color &color = Color(255));

    u64 spawn_line(u64 world, const glm::vec3 &a, const glm::vec3 &b, const Color &color, const glm::vec3 &position, const glm::quat &rotation);

    u64 spawn_chain(u64 world, const glm::vec3 *vertices, u32 num_vertices, Color &color, const glm::vec3 &position, const glm::quat &rotation);

    u64 spawn_polygon(u64 world, const glm::vec3 *vertices, u32 num_vertices, Color &color, const glm::vec3 &position, const glm::quat &rotation);

    u64 spawn_box(u64 world, f32 width, f32 height, const Color &top_color, const Color &bot_color, const glm::vec3 &position, const glm::quat &rotation, bool surface = false);

    u64 spawn_circle(u64 world, const glm::vec3 &center, f32 radius, const Color &color, const glm::vec3 &position, const glm::quat &rotation, bool surface = false);


    u64 spawn_actor_chain(u64 world, const glm::vec2 *vertices, u32 num_vertices,
                            bool kinematic, bool dynamic, bool gravity, bool trigger,
                            const char *collision_filter, const char *material,
                            const glm::vec3 &position, const glm::quat &rotation);

    u64 spawn_actor_polygon(u64 world, const glm::vec2 *vertices, u32 num_vertices,
                            bool kinematic, bool dynamic, bool gravity, bool trigger,
                            const char *collision_filter, const char *material,
                            const glm::vec3 &position, const glm::quat &rotation);

    u64 spawn_actor_circle(u64 world, f32 radius,
                           bool kinematic, bool dynamic, bool gravity, bool trigger,
                           const char *collision_filter, const char *material,
                           const glm::vec3 &position, const glm::quat &rotation);

    u64 spawn_actor_box(u64 world, f32 width, f32 height,
                        bool kinematic, bool dynamic, bool gravity, bool trigger,
                        const char *collision_filter, const char *material,
                        const glm::vec3 &position, const glm::quat &rotation);

    // Destroys the 'unit' from the 'world'
    void despawn_unit(u64 world, u64 unit);

    // Destroys the 'camera' from the 'world'
    void despawn_camera(u64 world, u64 camera);

    // Destroys the 'sprite' from the 'world'
    void despawn_sprite(u64 world, u64 sprite);

    // Destroys the 'text' from the 'world'
    void despawn_text(u64 world, u64 text);

    void despawn_geometry(u64 world, u64 geometry);

    void despawn_actor(u64 world, u64 actor);


    // returns the number of units in the 'world'
    u32 num_units(u64 world);

    // Returns all the units of the 'world'.
    void get_units(u64 world, Array<u64> &units);

    // Links the 'child' unit root to the 'node' of the 'parent' unit.
    void link_unit(u64 world, u64 child, u64 parent, i32 node);

    // Unlinks the 'child' unit.
    void unlink_unit(u64 world, u64 child);

    // Loads a 'level' into the world at the specified 'position' and 'rotation'.
    u64 load_level(u64 world, const char *name, const glm::vec3 &position, const glm::quat &rotation);

    // Destroys the specified 'level' and all units spawned when the level was loaded.
    void unload_level(u64 world, u64 level);

    // Enables/Disables physics simulation
    void physics_simulations(u64 world, bool value);

    u64 spawn_particles(u64 world, const char *effect, const glm::vec3 &position, const glm::quat &rotation);

    void despawn_particles(u64 world, u64 effect);

    void stop_spawning_particles(u64 world, u64 effect);

    void move_particles(u64 world, u64 effect, const glm::vec3 &position, const glm::quat &rotation);

    u32 num_particles(u64 world, u64 effect);
  }

  namespace window
  {
    void create  (const char *title, i32 width, i32 height, bool full_screen, bool resizable);
    void destroy (void);

    void get_resolution        (i32 &width, i32 &height);
    void get_screen_resolution (i32 &width, i32 &height);

    void set_on_resized (WindowResizeFun func);
    void get_size       (i32 &width, i32 &height);
    void set_size       (i32 &width, i32 &height);
    void set_title      (const char *title);
    void display_cursor (bool display);
  }

  namespace unit
  {
    void play_animation(u64 world, u64 unit, const char *animation, f32 from = 0.0f, f32 to = 0.0f, bool loop = false, f32 speed = 1.0f);
    bool is_playing_animation(u64 world, u64 unit);

    // Returns the world position, rotation or pose of a node in the unit.
    void get_world_position(u64 world, u64 unit, i32 node, glm::vec3 &v);
    void get_world_rotation(u64 world, u64 unit, i32 node, glm::quat &q);
    void get_world_scale(u64 world, u64 unit, i32 node, glm::vec3 &v);
    void get_world_pose(u64 world, u64 unit, i32 node, glm::mat4 &m);

    // Returns the local position, rotation or pose of a node in the unit.
    void get_local_position(u64 world, u64 unit, i32 node, glm::vec3 &v);
    void get_local_rotation(u64 world, u64 unit, i32 node, glm::quat &q);
    void get_local_scale(u64 world, u64 unit, i32 node, glm::vec3 &v);
    void get_local_pose(u64 world, u64 unit, i32 node, glm::mat4 &m);

    // Sets the local position, rotation or pose of a node in the unit.
    void set_local_position(u64 world, u64 unit, i32 node, const glm::vec3 &v);
    void set_local_rotation(u64 world, u64 unit, i32 node, const glm::quat &q);
    void set_local_scale(u64 world, u64 unit, i32 node, const glm::vec3 &v);
    void set_local_pose(u64 world, u64 unit, i32 node, const glm::mat4 &m);

    // Returns the index of the actor with the specified 'name', or -1 if there is no such actor.
    i32 actor_index(u64 world, u64 unit, const char *name);

    // Returns the physics actor with the specified 'name'.
    u64 actor(u64 world, u64 unit, const char *name);

    // Returns the i’th actor in the unit.
    u64 actor(u64 world, u64 unit, i32 i);

    // Returns the index of the mover with the specified 'name', or -1 if there is no such mover.
    i32 mover_index(u64 world, u64 unit, const char *name);

    // Returns the mover with the specified 'name'.
    u64 mover(u64 world, u64 unit, const char *name);

    // Returns the i’th mover in the unit.
    u64 mover(u64 world, u64 unit, i32 i);

    // Returns the index of the sprite with the specified 'name', or -1 if there is no such mover.
    i32 sprite_index(u64 world, u64 unit, const char *name);

    // Returns the sprite with the specified 'name'.
    u64 sprite(u64 world, u64 unit, const char *name);

    // Returns the i’th mover in the unit.
    u64 sprite(u64 world, u64 unit, i32 i);

    // Returns name if the unit is of the type name, otherwise returns 0.
    const char *is_a(u64 world, u64 unit, const char *name);

    // Resturns a hash representation of the unit name.
    u32 name_hash(u64 world, u64 unit);

    // Returns the box encapsulating all the unit component boxes.
    void box(u64 world, u64 unit, Box &box);
  }

  namespace camera
  {
    /*
    // Returns the camera’s unit.
    u64 unit(u64 world, u64 camera);

    // Returns the camera’s node index in the unit scene graph.
    u32 node(u64 world, u64 camera);
    */

    void get_local_translation(u64 world, u64 camera, glm::vec3 &t);
    void get_local_rotation(u64 world, u64 camera, glm::quat &m);
    void get_near_range(u64 world, u64 camera, f32 &v);
    void get_far_range(u64 world, u64 camera, f32 &v);
    void get_vertical_fov(u64 world, u64 camera, f32 &v);
    void get_projection_type(u64 world, u64 camera, ProjectionType &v);
    void get_transformation_matrix(u64 world, u64 camera, glm::mat4 &m);

    void set_local_translation(u64 world, u64 camera, const glm::vec3 &t);
    void set_local_rotation(u64 world, u64 camera, const glm::quat &q);
    void set_near_range(u64 world, u64 camera, f32 value);
    void set_far_range(u64 world, u64 camera, f32 value);
    void set_vertical_fov(u64 world, u64 camera, f32 value);
    void set_projection_type(u64 world, u64 camera, ProjectionType type);

    void set_orthographic_projection(u64 world, u64 camera, f32 left, f32 right, f32 bottom, f32 top);

    // Converts a point p from screen coordinates to world coordinates. In screen coordinates, (x,z) are the pixel positions of the point on the screen and the y-coordinate indicates the depth into the screen where the point lies (the distance from the camera plane) in meters.
    void screen_to_world(u64 world, u64 camera, const glm::vec3 &p, glm::vec3 &w);

    // Performs the inverse operation of screen_to_world(), converts from world coordinates to screen coordinates.
    // world_to_screen(camera, p) : Vector3

    // Tests whether the Vector3 p is inside the camera's frustum. A positive return value means the point is inside the frustum. The value specifies how many meters inside the frustum the point is. A negative value means the point is outside the frustum. The value specifies how far outside the frustum the point is.
    // inside_frustum(camera, p) : float
  }

  namespace sprite
  {
    
    // Returns the index of the 'name' frame, or -1
    i32  get_frame         (u64 world, u64 sprite, const char *name);

    // Returns the index of the setup frame
    i32  get_setup_frame   (u64 world, u64 sprite);

    // Returns the index of the current frame
    i32  get_current_frame (u64 world, u64 sprite);

    // Returns the number of the sprite contains
    i32  get_num_frames    (u64 world, u64 sprite);

    // Returns the size of the sprite for its current frame
    void get_size          (u64 world, u64 sprite, glm::vec2 &size);

    // Returns the blend color of the sprite
    void get_color         (u64 world, u64 sprite, Color &color);

    // Sets the frame with the specified index
    void set_frame         (u64 world, u64 sprite, i32 frame);

    // Sets the blend color of the sprite;
    void set_color         (u64 world, u64 sprite, const Color &color);

    // Returns the sprite’s world position, rotation or pose
    void get_world_position(u64 world, u64 sprite, glm::vec3 &v);
    void get_world_rotation(u64 world, u64 sprite, glm::quat &q);
    void get_world_scale(u64 world, u64 sprite, glm::vec3 &v);
    void get_world_pose(u64 world, u64 sprite, glm::mat4 &m);

    // Returns the sprite’s local position, rotation or pose
    void get_local_position(u64 world, u64 sprite, glm::vec3 &v);
    void get_local_rotation(u64 world, u64 sprite, glm::quat &q);
    void get_local_scale(u64 world, u64 sprite, glm::vec3 &v);
    void get_local_pose(u64 world, u64 sprite, glm::mat4 &p);

    // Sets the sprite’s local position, rotation or pose
    void set_local_position(u64 world, u64 sprite, const glm::vec3 &v);
    void set_local_rotation(u64 world, u64 sprite, const glm::quat &q);
    void set_local_scale(u64 world, u64 sprite, const glm::vec3 &v);
    void set_local_pose(u64 world, u64 sprite, const glm::mat4 &p);

    // Returns the AABB of the sprite.
    void box(u64 world, u64 sprite, Box &box);

    // Returns the unit that owns this actor. Note that some actors may not have an owning unit: 0 is returned.
    u64 unit(u64 world, u64 sprite);

    // Returns true if the sprite was visible the last time it's world was rendered.
    bool is_visible(u64 world, u64 sprite);
  }

  namespace text
  {
    /*
    // Returns the unit that owns this text or 0 if there is none
    u64 unit(u64 world, u64 text);

    // Returns the text’s node index in the unit scene graph or -1 if no unit owns the text
    i32 node(u64 world, u64 text);
    */

    // Returns the text’s width and height
    void get_size(u64 world, u64 text, glm::vec2 &v);
    void get_width(u64 world, u64 text, f32 &v);
    void get_height(u64 world, u64 text, f32 &v);

    // Sets the text’s string, font or alignment
    void set(u64 world, u64 text, const char*font, const char *string, TextAlign align, f32 scale = 1.f, const Color &color = Color(255));
    void set_string(u64 world, u64 text, const char *string);
    void set_font(u64 world, u64 text, const char*font);
    void set_alignment(u64 world, u64 text, TextAlign align);
    void set_scale(u64 world, u64 text, f32 scale);
    void set_color(u64 world, u64 text, const Color &color);

    // Returns the text’s world position, rotation or pose
    void get_world_position(u64 world, u64 text, glm::vec3 &v);
    void get_world_rotation(u64 world, u64 text, glm::quat &q);
    void get_world_scale(u64 world, u64 text, glm::vec3 &v);
    void get_world_pose(u64 world, u64 text, glm::mat4 &m);

    // Returns the text’s local position, rotation or pose
    void get_local_position(u64 world, u64 text, glm::vec3 &v);
    void get_local_rotation(u64 world, u64 text, glm::quat &q);
    void get_local_scale(u64 world, u64 text, glm::vec3 &v);
    void get_local_pose(u64 world, u64 text, glm::mat4 &p);

    // Sets the text’s local position, rotation or pose
    void set_local_position(u64 world, u64 text, const glm::vec3 &v);
    void set_local_rotation(u64 world, u64 text, const glm::quat &q);
    void set_local_scale(u64 world, u64 text, const glm::vec3 &v);
    void set_local_pose(u64 world, u64 text, const glm::mat4 &p);
  }

  namespace physics
  {
    void show_debug      (bool value);
    u64  create_raycast  (u64 world, RaycastCallback callback, const char *filter = 0, bool closest = true, bool any = false);
    void cast_raycast    (u64 world, u64 raycast, const glm::vec3 &from, const glm::vec3 &to);
    void destroy_raycast (u64 world, u64 raycast);
  }

  namespace actor
  {
    // Returns true if the actor is static.
    bool is_static(u64 world, u64 actor);

    // Returns true if the actor is kinematic(keyframed).
    bool is_kinematic(u64 world, u64 actor);

    // Returns true if the actor is non-static, i.e. either kinematic/keyframed or physical (physics driven).
    bool is_dynamic(u64 world, u64 actor);

    // Returns true if the actor is physical (physics driven).
    bool is_physical(u64 world, u64 actor);

    //Returns the velocity of the actor.
    void get_velocity(u64 world, u64 actor, glm::vec3 &v);

    //Sets the velocity of the actor. (This call, and all the other calls that change velocity or add forces, only affect physical actors.)
    void set_velocity(u64 world, u64 actor, const glm::vec3 &v);
    
    void set_touched_callback   (u64 world, u64 actor, ContactCallback callback, const void *user_data);
    void set_untouched_callback (u64 world, u64 actor, ContactCallback callback, const void *user_data);

    // Returns the actor’s world position, rotation or pose
    void get_world_position(u64 world, u64 actor, glm::vec3 &v);
    void get_world_rotation(u64 world, u64 actor, glm::quat &q);
    void get_world_scale(u64 world, u64 actor, glm::vec3 &v);
    void get_world_pose(u64 world, u64 actor, glm::mat4 &m);

    // Returns the actor’s local position, rotation or pose
    void get_local_position(u64 world, u64 actor, glm::vec3 &v);
    void get_local_rotation(u64 world, u64 actor, glm::quat &q);
    void get_local_scale(u64 world, u64 actor, glm::vec3 &v);
    void get_local_pose(u64 world, u64 actor, glm::mat4 &p);

    // Sets the actor’s local position, rotation or pose
    void set_local_position(u64 world, u64 actor, const glm::vec3 &v);
    void set_local_rotation(u64 world, u64 actor, const glm::quat &q);
    void set_local_scale(u64 world, u64 actor, const glm::vec3 &v);
    void set_local_pose(u64 world, u64 actor, const glm::mat4 &p);

    // Makes the actor kinematic, i.e. keyframed.
    void set_kinematic(u64 world, u64 actor);

    // Makes the actor non-kinematic, i.e., not keyframed.
    void clear_kinematic(u64 world, u64 actor);

    //Adds a momentary impulse (mass * velocity) to the actor.
    void add_impulse(u64 world, u64 actor, const glm::vec3 &impulse);

    // Returns the unit that owns this actor. Note that some actors may not have an owning unit.
    u64 unit(u64 world, u64 actor);

    //Changes the collision filter used by the actor.
    void set_collision_filter(u64 world, u64 actor, const char *filter);

    //Actor

    /*
    bool collision_disabled(Actor actor);
    void disable_collision(Actor actor);
    void enable_collision(Actor actor);

    bool gravity_disabled(Actor actor);
    void disable_gravity(Actor actor);
    void enable_gravity(Actor actor);

    /*
    //Returns the linear damping of the actor.
    f32 linear_damping(Actor actor);

    //Sets the linear damping of the actor to f.
    void set_linear_damping(Actor actor, f32 f);

    //Returns the angular damping of the actor.
    f32 angular_damping(Actor actor);

    //Sets the angular damping of the actor.
    void set_angular_damping(Actor actor, f32 f);

    //Returns the angular velocity of the actor.
    glm::vec3 angular_velocity(Actor actor);

    //Sets the angular velocity of the actor to v.
    void set_angular_velocity(Actor actor, glm::vec2 v);

    //Returns the velocity at a point p on the surface of the actor. The point is specified in global coordinates.
    f32 point_velocity(Actor actor, glm::vec2 point);

    //Adds a momentary impulse (mass * velocity) to the actor.
    void add_impulse(Actor actor, f32 impulse);

    //As add_impulse(), but applies the impulse at a particular position in the actor, causing rotation.
    void add_impulse_at(Actor actor, f32 impulse, glm::vec2 pos);

    //Adds a delta velocity to the actor’s velocity.
    void add_velocity(Actor actor, f32 delta_velocity);

    //As above, but applies it at position pos, causing rotation.
    void add_velocity_at(Actor actor, f32 delta_velocity, glm::vec2 pos);

    //Adds a rotational impulse to the actor.
    void add_torque_impulse(Actor actor, f32 impulse);

    //Adds a delta angular velocity.
    void add_angular_velocity(Actor actor, f32 delta_velocity);

    //Pushes the actor as if it was hit by an object with velocity and mass. Use this to simulate collision with objects that are not physics simulated, such as bullets.
    void push(Actor actor, f32 velocity, f32 mass);

    //As push, but applies the force at a particular position, causing rotation.
    void push_at(Actor actor, f32 velocity, f32 mass, glm::vec2 pos);

    //Returns true if the actor is sleeping.
    bool is_sleeping(Actor actor);

    //Wakes the actor up.
    void wake_up(Actor actor);

    //Returns the unit that owns this actor. Note that some actors may not have an owning unit.
    u64 unit(Actor actor);

    //Changes the collision filter used by the actor.
    void set_collision_filter(const char ** filter);
    */
  }

  namespace mover
  {
    // Attempts to move the mover by the specified position. The mover will slide against physical objects.
    void move (u64 world, u64 mover, const glm::vec3 &delta_position);

    // Returns the position of the mover.
    void get_position (u64 world, u64 mover, glm::vec3 &position);

    // Teleports tthe mover to the specified position.
    void set_position (u64 world, u64 mover, const glm::vec3 &position);

    // Returns the unit that owns the mover.
    u64 get_unit (u64 world, u64 mover);

    // Returns true if the mover is standing.
    bool collides_down (u64 world, u64 mover);

    // Returns true if the mover hitting its head.
    bool collides_up (u64 world, u64 mover);

    // Returns true if the mover is colliding horizontally.
    bool collides_sides (u64 world, u64 mover);

    // Returns true if the mover is colliding on the left side.
    bool collides_right (u64 world, u64 mover);

    // Returns true if the mover is colliding on the right side.
    bool collides_left (u64 world, u64 mover);

    // Changes the collision filter used by the mover
    void set_collision_filter (u64 world, u64 mover, const char *filter);
  }

  namespace math
  {
    // Computes the intersection between the ray (from, dir) and the box. 
    // If the ray collides with any of the planes of the box, the distance to the collision point along the ray is returned 
    // (regardless of whether the ray started inside or outside the box). 
    // If there is no collision, -1 is returned.
    f32 ray_box_intersection(const glm::vec3 &from, const glm::vec3 &dir, const Box &box);
  }

  namespace geometry
  {
    void get_local_position(u64 world, u64 geometry, glm::vec3 &v);
    void get_local_rotation(u64 world, u64 geometry, glm::quat &q);
    void get_local_scale(u64 world, u64 geometry, glm::vec3 &v);
    void get_local_pose(u64 world, u64 geometry, glm::mat4 &p);
    
    void set_local_position(u64 world, u64 geometry, const glm::vec3 &v);
    void set_local_rotation(u64 world, u64 geometry, const glm::quat &q);
    void set_local_scale(u64 world, u64 geometry, const glm::vec3 &v);
    void set_local_pose(u64 world, u64 geometry, const glm::mat4 &p);
  }

  namespace resource_package
  {
    // Starts loading the resources in the background.
    void load(u64 package);

    // Returns true if the background thread has completed loading the package.
    bool has_loaded(u64 package);

    // Brings the resources on the package online. 
    // If the package has not completed loading, this call will stall until the package has been loaded.
    void flush(u64 package);

    // Unloads the resources in the package. 
    // You must make sure that the resources are no longer in use before unloading them.
    void unload(u64 package);
  }

  namespace pad
  {
    // Returns true if the controller is connected and working.
    bool active(u32 pad);

    // Returns true if the controller was attached this frame.
    bool connected(u32 pad);

    // Returns true if the controller was removed this frame.
    bool disconnected(u32 pad);

    // Returns true if the current value button is press
    bool button(u32 pad, u32 key);

    // Returns true if the button was pressed this frame.
    bool pressed(u32 pad, u32 key);

    // Returns true if any button was pressed this frame.
    bool any_pressed(u32 pad);

    // Returns true if the button was released this frame.
    bool released(u32 pad, u32 key);

    // Returns true if any button was released this frame.
    bool any_released(u32 pad);

    // Returns a string with the name of the controller
    const char *name(u32 pad);

    // Returns the number of buttons on the controller.
    i32 num_buttons(u32 pad);

    // Returns the number of axes on the controller
    i32 num_axes(u32 pad);

    // Returns the value of the i’th axis on the controller.
    f32 axes(u32 pad, u32 i);

    /*
    // Returns the current down threshold. Default is 0.5.
    f32 down_threshold(u32 pad);

    // Set the threshold([0,1]) of when a button is considered 'down'. Default is 0.5. Larger values will result in the user has to press the button harder in order indicate it as 'down'.
    void set_down_threshold(u32 pad, f32 value);

    // Returns the name of the i’th button.
    button_name(i) : string

    // For keyboards this returns the name of the i’th button with respect to the keyboard type (locale). This string may be empty if there is no name for the key. Shift and alt for example will be empty strings.
    button_locale_name(i) : string

    // Returns the index of the button with the specified name, or nil if no such button is found.
    button_index(name) : int

    // Returns the name of the i’th axis.
    axis_name(i) : string

    // Returns the index of the axis with the specified name or nil if no such axis is found.
    axis_index(name) : int

    // rumble ....
    */
  }

  namespace mouse
  {
    // Returns mouse position
    glm::vec2 get_position(void);

    // Sets mouse position
    void set_position(glm::vec2 &position);

    // Returns true if the current value button is press
    bool button(u32 key);

    // Returns true if the button was pressed this frame.
    bool pressed(u32 key);

    // Returns true if any button was released this frame.
    bool released(u32 key);

    // Returns true if any button was pressed this frame.
    bool any_pressed(void);

    // Returns true if any button was released this frame.
    bool any_released(void);

    // Returns wheel's scroll value
    i32 wheel_scroll(void);
  }

  namespace keyboard
  {
    // Returns true if the current value button is press
    bool button(u32 key);

    // Returns true if the button was pressed this frame.
    bool pressed(u32 key);

    // Returns true if any button was released this frame.
    bool released(u32 key);

    // Returns true if any button was pressed this frame.
    bool any_pressed(void);

    // Returns true if any button was released this frame.
    bool any_released(void);
  }

  namespace audio
  {
    // Plays the sound resource name as a 2D sound.
    u64 trigger_sound(u64 world, const char *name);

    // Plays the sound resource name in the world and returns an id to it. 
    // u64 trigger_sound(u64 world, const char *name, const glm::vec3 &position);

    // Plays the sound resource name in the world and returns an id to it. 
    // Links the position of the sound to the position of the node in the unit.
    // u64 trigger_sound(u64 world, const char *name, u64 unit, i32 node);

    // Stops the sound with the specified id.
    void stop(u64 world, u64 id);

    // Stops all sounds playing in the world.
    void stop_all(u64 world);

    // Sets the audio listener to the specified position
    // void set_listener(u64 world, const glm::vec3 &position);

    // Sets the volume of the specified bus.
    // 1.0 means full strength, 0.0 means it's muted.
    // void set_bus_volume(u64 world, const char *name, f32 volume);

    // Sets the reverb environment for sounds.
    // void set_environment(u64 world, const char *name);
  }
}