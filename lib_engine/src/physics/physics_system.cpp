#include <runtime/types.h>
#include <runtime/memory.h>
#include <runtime/idlut.h>
#include <runtime/trace.h>

#include <resource/resource_manager.h>
#include <resource/unit_resource.h>
#include <scene/scene_system.h>
#include <runtime/hash.h>
#include <runtime/murmur_hash.h>
#include <runtime/temp_allocator.h>

#include <engine/pge_types.h>
#include "physics_resource.h"
#include "actor_resource.h"
#include "shape_resource.h"
#include "physics_wrapper.h"
#include "physics_system.h"

#include <glm/gtx/vector_angle.hpp>

namespace
{
  using namespace pge;

  static i32 ppm;

  // Physics Globals
  Allocator *_a = NULL;
  Hash<ActorTemplate>    *actors    = NULL;
  Hash<MaterialTemplate> *materials = NULL;
  Hash<ShapeTemplate>    *shapes    = NULL;
  Hash<CollisionFilter>  *collision_filters = NULL;

  void create_shapes(PhysicsWorld &world, u64 actor_id, const ActorData::Shape *shapes, u32 num_shapes, u32 group)
  {
    ShapeType type;
    Array<f32> data(*_a);
    f32 w, h, x0, y0;
    glm::vec4 res;
    glm::mat4 m;

    const ActorData::Shape *shape_res, *end = (shapes + num_shapes);

    for (shape_res = shapes; shape_res < end; shape_res++) {
      type = shape_res->shape->type;

      // Applies shape's pose and PPM
      u32  num_components   = shape_res->shape->num_components;
      const f32 *components = shape_res->shape->components;
      array::resize(data, num_components);

      compose_mat4(m, shape_res->pose);

      switch (type) {
        case SHAPE_TYPE_CIRCLE:
          data[0] = components[0] / ppm;
          break;
        case SHAPE_TYPE_BOX:
          type = SHAPE_TYPE_POLYGON;
          num_components = 8;
          array::resize(data, num_components);

          w = components[0];
          h = components[1];
          x0 = w * .5f;
          y0 = h * .5f;

          //apply pose
          res = m * glm::vec4(-x0, -y0, .0f, 1.f);
          data._data[0] = res.x / ppm;
          data._data[1] = res.y / ppm;
          res = m * glm::vec4(w - x0, -y0, .0f, 1.f);
          data._data[2] = res.x / ppm;
          data._data[3] = res.y / ppm;
          res = m * glm::vec4(w - x0, h - y0, .0f, 1.f);
          data._data[4] = res.x / ppm;
          data._data[5] = res.y / ppm;
          res = m * glm::vec4(-x0, h - y0, .0f, 1.f);
          data._data[6] = res.x / ppm;
          data._data[7] = res.y / ppm;
          break;
        case SHAPE_TYPE_CHAIN:
        case SHAPE_TYPE_POLYGON:
        {
          u32 num_vert = (u32)(num_components * .5f);
          for (u32 i = 0; i < num_vert; i++) {
            res = m * glm::vec4(components[(2 * i)], components[(2 * i) + 1], .0f, 1.f);
            data._data[(2 * i)]     = res.x / ppm;
            data._data[(2 * i) + 1] = res.y / ppm;
          }
        }
          break;
        default:
          XERROR("Shape type %d not handled", type);
      }

      // Creates shape

      Shape *shape = NULL;
      glm::vec2 p1, p2;
      switch (type) {
        case SHAPE_TYPE_CIRCLE:
          shape = physics::create_circle(data[0], glm::vec2(shape_res->pose.translation.x / ppm, shape_res->pose.translation.y / ppm));
          break;
        case SHAPE_TYPE_POLYGON:
          shape = physics::create_polygon((glm::vec2*)array::begin(data), (i32)(num_components*.5f));
          break;
        case SHAPE_TYPE_CHAIN:
          shape = physics::create_chain((const glm::vec2 *)array::begin(data), i32(array::size(data)*.5f));
          break;
        default:
          XERROR("Shape type %d not handled.", type);
      }

      // Creates Fixture

      const CollisionFilter *filter = hash::get(*collision_filters, shape_res->tpl->collision_filter);

      ShapeInfo shape_info;
      shape_info.actor   = actor_id;
      shape_info.type    = type;
      shape_info.fixture = physics::create_fixture(idlut::lookup(world.actors, actor_id)->body,
                                                   shape,
                                                   shape_res->material->density,
                                                   shape_res->material->friction,
                                                   shape_res->material->restitution);

      physics::set_trigger(shape_info.fixture, shape_res->tpl->trigger == 1u);
      physics::set_filter(shape_info.fixture, filter->is, filter->collides_with);
      physics::set_group(shape_info.fixture, group);

      hash::set(world.shape_infos, (u64)shape, shape_info);
      multi_hash::insert(world.actor_shapes, actor_id, shape);
    }
  }

  void rescale_or_flip_shapes(PhysicsWorld &w, u64 actor_id, const glm::vec2 &scale, bool flip) {
    const Actor      &actor = (*idlut::lookup(w.actors, actor_id));

    TempAllocator1024 ta(*w.actors._data._allocator);
    Array<Shape*>     old_shapes(ta);
    Array<Shape*>     new_shapes(ta);

    Array<glm::vec2>  vertices(ta);
    glm::vec2         t;

    multi_hash::get(w.actor_shapes, actor_id, old_shapes);
    array::resize(new_shapes, array::size(old_shapes));

    for (u32 i = 0; i < array::size(old_shapes); i++) {
      const ShapeInfo shape_info = *hash::get(w.shape_infos, (u64)old_shapes[i]);

      switch (shape_info.type) {
        case SHAPE_TYPE_CIRCLE:
        {
          physics::get_shape_position(old_shapes[i], t);
          if (flip) t.x *= -1;
          new_shapes[i] = physics::create_circle(physics::get_circle_radius(old_shapes[i]) * (scale.x + scale.y)/2, t * scale);
        }
          break;
        case SHAPE_TYPE_CHAIN:
        case SHAPE_TYPE_BOX:
        case SHAPE_TYPE_POLYGON:
        {
          u32 num_vertices = physics::get_num_vertices(old_shapes[i]);
          const glm::vec2 *phys_vert = physics::get_vertices(old_shapes[i]);

          array::resize(vertices, num_vertices);

          for (u32 j = 0; j < num_vertices; j++) {
            vertices[j] = phys_vert[j] * scale;
            if (flip) vertices[j].x *= -1;
          }

          if (shape_info.type == SHAPE_TYPE_CHAIN)
            new_shapes[i] = physics::create_chain(vertices._data, num_vertices);
          else
            new_shapes[i] = physics::create_polygon(vertices._data, num_vertices);
        }
          break;
        default:
          XERROR("Shape type \"%d\" not handled!", shape_info.type);
      }

      ShapeInfo new_shape_info;
      new_shape_info = shape_info;
      new_shape_info.fixture = physics::create_fixture(actor.body, new_shapes[i], shape_info.fixture);
      hash::set(w.shape_infos, (u64)new_shapes[i], new_shape_info);

      physics::destroy_shape(old_shapes[i]);
      physics::destroy_fixture(actor.body, shape_info.fixture);
      hash::remove(w.shape_infos, (u64)old_shapes[i]);
    }

    multi_hash::remove_all(w.actor_shapes, actor_id);
    for (u32 i = 0; i < array::size(new_shapes); i++)
      multi_hash::insert(w.actor_shapes, actor_id, new_shapes[i]);
  }

  void destroy_shapes(PhysicsWorld &w, u64 actor_id)
  {
    const Actor &actor = (*idlut::lookup(w.actors, actor_id));
    const Hash<Shape*>::Entry *e = multi_hash::find_first(w.actor_shapes, actor_id);

    while (e) {
      physics::destroy_shape(e->value);
      physics::destroy_fixture(actor.body, hash::get(w.shape_infos, (u64)e->value)->fixture);
      hash::remove(w.shape_infos, (u64)e->value);
      e = multi_hash::find_next(w.actor_shapes, e);
    }
    multi_hash::remove_all(w.actor_shapes, actor_id);
  }

  // WORLD

  struct ContactData
  {
    PhysicsWorld *w;
    Array<ContactPoint> *data;
  };

  static void push_actor_contact(const glm::vec2 &position, f32 distance, const glm::vec2 &normal, const Body *body, void *data)
  {
    ContactData &cd = *(ContactData*)data;
    ContactPoint cp;
    cp.actor = *hash::get(cd.w->body_actor, (u64)body);
    cp.position = glm::vec3(position * (f32)ppm, 0);
    cp.distance = distance * (f32)ppm;
    cp.normal   = glm::vec3(normal * (f32)ppm, 0);

    array::push_back(*cd.data, cp);
  }

  struct MoverData
  {
    glm::vec2 pos;
    Mover *mover;
  };

  static void update_mover_collision_state(const glm::vec2 &position, f32 distance, const glm::vec2 &normal, const Body *body, void *data)
  {
    (void)body;
    (void)normal;
    (void)distance;

    MoverData &m = *(MoverData*)data;
    f32 y = position.y * ppm - m.pos.y;
    f32 x = position.x * ppm - m.pos.x;
    if (y < m.mover->res->radius + m.mover->res->offset[1] && abs(x) < (m.mover->res->radius - 1))
      m.mover->collides_down = true;
    else if (y > m.mover->res->height + m.mover->res->offset[1] + m.mover->res->radius && abs(x) < (m.mover->res->radius - 1))
      m.mover->collides_up = true;
    else if (x > 0)
      m.mover->collides_right = true;
    else
      m.mover->collides_left = true;
  }

  struct RaycastData
  {
    Array<ContactPoint> *hits;
    PhysicsWorld *world;
  };

  void push_raycast_hit(const glm::vec2 &position, f32 distance, const glm::vec2 &normal, const Body *body, void *data)
  {
    RaycastData &rdata = *(RaycastData*)data;
    ContactPoint cp;
    cp.distance = distance * (f32)ppm;
    cp.actor    = *hash::get(rdata.world->body_actor, (u64)body);
    cp.normal   = glm::vec3(normal * (f32)ppm, 0);
    cp.position = glm::vec3(position * (f32)ppm, 0);
    array::push_back(*rdata.hits, cp);
  }

  void update_wrapper_poses(PhysicsWorld &w) {
    DecomposedMatrix dm;
    IdLookupTable<Actor>::Entry *e, *end = idlut::end(w.actors);
    
    const glm::vec2 ref(1, 0);
    glm::vec2       dir;
    glm::vec4       tmp;
    bool            flip;

    // update physics wrapper poses
    for (e = idlut::begin(w.actors); e < end; e++) {
      Actor &a = e->value;

      if (!pose::is_dirty(a.pose))
        continue;

      pose::update(a.pose);
      decompose_mat4(a.pose._world, dm);

      tmp  = a.pose._world * glm::vec4(0, 0, 0, 1);
      flip = tmp.z - (a.pose._world * glm::vec4(0, 0, 1, 1)).z > 0;
      dir  = glm::normalize(glm::vec2(a.pose._world * glm::vec4(flip ? -1 : 1, 0, 0, 1) - tmp));

      physics::set_body_position(a.body, glm::vec2(dm.translation.x / ppm, dm.translation.y / ppm));
      physics::set_body_rotation(a.body, glm::orientedAngle(ref, dir));
      physics::set_awake(a.body, true);

      dm.scale = glm::abs(dm.scale);
      if (a.old_scale != dm.scale || a.old_flip != flip) {
        rescale_or_flip_shapes(w, e->id, glm::vec2((dm.scale / a.old_scale)), a.old_flip != flip);
        a.old_scale = dm.scale;
        a.old_flip  = flip;
      }
    }
  }
}

namespace pge
{
  namespace physics_system
  {
    // ---------------------------------------------------------------
    // Global system functions
    // ---------------------------------------------------------------

    void init(u32 res_name, Allocator &a)
    {
      _a = &a;
      const PhysicsResource *res = (PhysicsResource*)resource_manager::get(RESOURCE_TYPE_PHYSICS, res_name);

      physics::initialize(1, 1);
      ppm = physics_resource::get_ppm(res);

      actors    = MAKE_NEW(a, Hash<ActorTemplate>, a);
      materials = MAKE_NEW(a, Hash<MaterialTemplate>, a);
      shapes    = MAKE_NEW(a, Hash<ShapeTemplate>, a);
      collision_filters = MAKE_NEW(a, Hash<CollisionFilter>, a);

      u32 num_actors    = physics_resource::num_actors(res);
      u32 num_materials = physics_resource::num_materials(res);
      u32 num_shapes    = physics_resource::num_shapes(res);
      u32 num_collision_filter = physics_resource::num_collision_filters(res);

      hash::reserve(*actors, num_actors);
      hash::reserve(*materials, num_materials);
      hash::reserve(*shapes, num_shapes);
      hash::reserve(*collision_filters, num_collision_filter);

      const PhysicsResource::Actor *actor = physics_resource::actors(res);
      for (u32 i = 0; i < num_actors; i++) {
        ActorTemplate tpl;
        tpl.kinematic = actor->kinematic == 1u;
        tpl.dynamic   = actor->dynamic == 1u;
        tpl.gravity   = actor->disable_gravity == 0u;

        hash::set(*actors, (u64)actor->name, tpl);
        actor++;
      }

      const PhysicsResource::Material *material = physics_resource::materials(res);
      for (u32 i = 0; i < num_materials; i++) {
        MaterialTemplate tpl;
        tpl.density     = material->density;
        tpl.friction    = material->friction;
        tpl.restitution = material->restitution;

        hash::set(*materials, (u64)material->name, tpl);
        material++;
      }

      const PhysicsResource::Shape *shape = physics_resource::shapes(res);
      for (u32 i = 0; i < num_shapes; i++) {
        ShapeTemplate tpl;
        tpl.collision_filter = shape->collision_filter;
        tpl.trigger = shape->trigger;

        hash::set(*shapes, (u64)shape->name, tpl);
        shape++;
      }

      const PhysicsResource::CollisionFilter *filter = physics_resource::collision_filters(res);
      for (u32 i = 0; i < num_collision_filter; i++) {
        CollisionFilter tpl;
        tpl.collides_with = filter->collides_with;
        tpl.is = filter->is;

        hash::set(*collision_filters, filter->name, tpl);
        filter++;
      }
    }

    i32 get_ppm()
    {
      return ppm;
    }

    const ActorTemplate &get_actor_template(u32 name)
    {
      return *hash::get(*actors, name);
    }

    const MaterialTemplate &get_material_template(u32 name)
    {
      return *hash::get(*materials, name);
    }

    const ShapeTemplate &get_shape_template(u32 name)
    {
      return *hash::get(*shapes, name);
    }

    const CollisionFilter &get_collision_filter(u32 name)
    {
      return *hash::get(*collision_filters, name);
    }

    void shutdown(void)
    {
      MAKE_DELETE((*_a), Hash<ActorTemplate>, actors);
      MAKE_DELETE((*_a), Hash<MaterialTemplate>, materials);
      MAKE_DELETE((*_a), Hash<ShapeTemplate>, shapes);
      MAKE_DELETE((*_a), Hash<CollisionFilter>, collision_filters);
    }


    // ---------------------------------------------------------------
    // Actor Manipulation
    // ---------------------------------------------------------------

    u64 create_actor(PhysicsWorld &w, const ActorData &actor_data, u32 group)
    {
      Actor     actor;
      u64       actor_id;

      actor.dynamic   = actor_data.actor->dynamic;
      actor.kinematic = actor_data.actor->kinematic;
      actor.gravity   = actor_data.actor->gravity;
      actor.old_scale = glm::vec3(1);
      actor.old_flip  = false;

      actor.body = !actor.dynamic && !actor.kinematic
        ? physics::create_body(w.space, 0, 0, BODY_TYPE_STATIC)
        : actor.kinematic
        ? physics::create_body(w.space, 0, 0, BODY_TYPE_KINEMATIC)
        : physics::create_body(w.space, 0, 0, BODY_TYPE_DYNAMIC);

      if (actor.gravity)
        physics::enable_gravity(actor.body);
      else
        physics::disable_gravity(actor.body);

      actor_id = idlut::add(w.actors, actor);
      hash::set(w.body_actor, (u64)actor.body, actor_id);

      create_shapes(w, actor_id, actor_data.shapes, actor_data.num_shapes, group);

      return actor_id;
    }

    void destroy_actor(PhysicsWorld &w, u64 actor)
    {
      Allocator &a = *w.actors._data._allocator;

      // remove actor shapes
      destroy_shapes(w, actor);
      // remove actor from the world
      Actor *pa = idlut::lookup(w.actors, actor);
      hash::remove(w.body_actor, (u64)pa->body);

      if (hash::has(w.listeners, actor)) {
        const ActorListener *l = hash::get(w.listeners, actor);
        MAKE_DELETE(a, Array<ContactPoint>, l->result);
        MAKE_DELETE(a, Array<ContactPoint>, l->contacts);
        MAKE_DELETE(a, Hash<ActorListener::PrevContact>, l->prev_contacts);
        hash::remove(w.listeners, actor);
      }

      physics::destroy_body(pa->body);
      idlut::remove(w.actors, actor);
    }


    bool is_static(PhysicsWorld &w, u64 actor)
    {
      const Actor &a = *idlut::lookup(w.actors, actor);
      return !a.dynamic && !a.kinematic;
    }

    bool is_dynamic(PhysicsWorld &w, u64 actor)
    {
      const Actor &a = *idlut::lookup(w.actors, actor);
      return a.dynamic || a.kinematic;
    }

    bool is_physical(PhysicsWorld &w, u64 actor)
    {
      const Actor &a = *idlut::lookup(w.actors, actor);
      return a.dynamic && !a.kinematic;
    }

    bool is_kinematic(PhysicsWorld &w, u64 actor)
    {
      return idlut::lookup(w.actors, actor)->kinematic;
    }


    void get_velocity(PhysicsWorld &w, u64 actor, glm::vec3 &v)
    {
      physics::get_velocity(idlut::lookup(w.actors, actor)->body, (glm::vec2&)v);
    }

    void set_velocity(PhysicsWorld &w, u64 actor, const glm::vec3 &v)
    {
      Actor &a = *idlut::lookup(w.actors, actor);
      physics::set_velocity(a.body, glm::vec2(v.x, v.y));
    }

    void set_touched_callback(PhysicsWorld &w, u64 actor, void(*callback)(const Array<ContactPoint> &contacts))
    {
      Allocator &a = *w.actors._data._allocator;
      ActorListener l ={ 0 };
      l = hash::get(w.listeners, actor, l);

      if (!l.result) {
        l.result        = MAKE_NEW(a, Array<ContactPoint>, a);
        l.contacts      = MAKE_NEW(a, Array<ContactPoint>, a);
        l.prev_contacts = MAKE_NEW(a, Hash<ActorListener::PrevContact>, a);
      }

      l.touched_callback = callback;
      hash::set(w.listeners, actor, l);
    }

    void set_untouched_callback(PhysicsWorld &w, u64 actor, void(*callback)(const Array<ContactPoint> &contacts))
    {
      Allocator &a = *w.actors._data._allocator;
      ActorListener l ={ 0 };
      l = hash::get(w.listeners, actor, l);

      if (!l.result) {
        l.result        = MAKE_NEW(a, Array<ContactPoint>, a);
        l.contacts      = MAKE_NEW(a, Array<ContactPoint>, a);
        l.prev_contacts = MAKE_NEW(a, Hash<ActorListener::PrevContact>, a);
      }

      l.untouched_callback = callback;
      hash::set(w.listeners, actor, l);
    }

    void add_impulse(PhysicsWorld &w, u64 actor, const glm::vec3 &impulse)
    {
      Actor &a = *idlut::lookup(w.actors, actor);
      physics::apply_impulse(a.body, glm::vec2(impulse));
    }

    void set_kinematic(PhysicsWorld &w, u64 actor, bool value)
    {
      Actor &a = *idlut::lookup(w.actors, actor);
      if (value) {
        physics::set_body_type(a.body, BODY_TYPE_KINEMATIC);
        a.kinematic = true;
      } else {
        a.kinematic = false;
        if (a.dynamic)
          physics::set_body_type(a.body, BODY_TYPE_DYNAMIC);
        else
          physics::set_body_type(a.body, BODY_TYPE_STATIC);
      }
    }

    void set_collision_filter(PhysicsWorld &w, u64 actor, const char *filter)
    {
      Actor *a = idlut::lookup(w.actors, actor);
      const CollisionFilter &cf = get_collision_filter(murmur_hash_32(filter));
      physics::set_body_filter(a->body, cf.is, cf.collides_with);
    }

    Pose &get_pose(PhysicsWorld &w, u64 actor)
    {
      return idlut::lookup(w.actors, actor)->pose;
    }

    // ---------------------------------------------------------------
    // Mover Manipulation
    // ---------------------------------------------------------------

    u64 create_mover(PhysicsWorld &w, const MoverResource *r, u32 group)
    {
      Mover m;
      m._dp    = glm::vec3(0, 0, 0);
      m._dirty = false;
      m.collides_down = m.collides_up = m.collides_left = m.collides_right = false;
      m.res = (MoverResource*)r;

      ActorTemplate actor_template;
      actor_template.kinematic = false;
      actor_template.dynamic   = true;
      actor_template.gravity   = false;

      MaterialTemplate material_template;
      material_template.density     = 0;
      material_template.friction    = 0;
      material_template.restitution = 0;

      ShapeTemplate shape_template;
      shape_template.trigger = false;
      shape_template.collision_filter = r->collision_filter ? r->collision_filter : physics::COLLIDES_WITH_ALL;

      ShapeData circle;
      f32       circle_components[1];
      circle_components[0]  = r->radius;
      circle.type           = SHAPE_TYPE_CIRCLE;
      circle.components     = circle_components;
      circle.num_components = 1;

      ShapeData rectangle;
      f32       rectangle_components[2];
      rectangle_components[0]  = r->radius * 2;
      rectangle_components[1]  = r->height;
      rectangle.type           = SHAPE_TYPE_BOX;
      rectangle.components     = rectangle_components;
      rectangle.num_components = 2;

      ActorData::Shape actor_shapes[3];

      actor_shapes[0].material = actor_shapes[1].material = actor_shapes[2].material = &material_template;
      actor_shapes[0].tpl      = actor_shapes[1].tpl      = actor_shapes[2].tpl      = &shape_template;
      actor_shapes[0].shape    = actor_shapes[2].shape    = &circle; // head and foots are circles
      actor_shapes[1].shape    = &rectangle;                         // body is a rectangle

      actor_shapes[0].pose.translation = glm::vec3(0, r->height + r->radius, 0);
      actor_shapes[1].pose.translation = glm::vec3(0, r->height / 2 + r->radius, 0);
      actor_shapes[2].pose.translation = glm::vec3(0, r->radius, 0);

      ActorData actor_data;
      actor_data.actor      = &actor_template;
      actor_data.num_shapes = 3;
      actor_data.shapes     = actor_shapes;

      m.capsule = physics_system::create_actor(w, actor_data, group);

      return idlut::add(w.movers, m);
    }

    void destroy_mover(PhysicsWorld &w, u64 mover)
    {
      Mover *m = idlut::lookup(w.movers, mover);

      destroy_actor(w, m->capsule);
      idlut::remove(w.movers, mover);
    }

    void get_mover_position(PhysicsWorld &w, u64 mover, glm::vec3 &p)
    {
      Mover *m = idlut::lookup(w.movers, mover);
      Actor *c = idlut::lookup(w.actors, m->capsule);
      glm::vec2 v;

      physics::get_body_position(c->body, v);
      p = glm::vec3(v, 0) * (f32)ppm;
    }

    void teleport_mover(PhysicsWorld &w, u64 mover, const glm::vec3 &p)
    {
      Mover *m = idlut::lookup(w.movers, mover);
      Actor *c = idlut::lookup(w.actors, m->capsule);
      physics::set_body_position(c->body, glm::vec2(p) / (f32)ppm);
      m->_dirty = true;
    }

    void move_mover(PhysicsWorld &w, u64 mover, const glm::vec3 &dp)
    {
      Mover *m = idlut::lookup(w.movers, mover);
      m->_dirty = true;
      m->_dp = dp;
    }

    void set_mover_collision_filter(PhysicsWorld &w, u64 mover, const char *filter)
    {
      Mover *m = idlut::lookup(w.movers, mover);
      set_collision_filter(w, m->capsule, filter);
    }

    // ---------------------------------------------------------------
    // World manipulation & querying
    // ---------------------------------------------------------------


    void allow_simulations(PhysicsWorld &w, bool value)
    {
      w.simulate = value;
    }

    void update(PhysicsWorld &w, f64 delta_time)
    {
      glm::mat4 m;
      DecomposedMatrix dm;
      IdLookupTable<Actor>::Entry *e, *end = idlut::end(w.actors);
      IdLookupTable<Mover>::Entry *me, *mend = idlut::end(w.movers);
      i32 num_steps = 1;
      w.accumulator += delta_time;

      /* fixed timestep pas possible tant que pas d'interpolation entre les états
        i32 num_steps = 0;

        while (w.accumulator >= w.step_time) {
        ++num_steps;
        w.accumulator -= w.step_time;
        }*/

      // update physics wrapper poses
      update_wrapper_poses(w);

      if (delta_time > 0.0 && num_steps && w.simulate) {
        // reset mover actor velocities if it has not been set by the user
        for (me = idlut::begin(w.movers); me < mend; me++) {
          Mover &m = me->value;
          Actor &a = *idlut::lookup(w.actors, m.capsule);

          if (m._dirty)
            physics::set_velocity(a.body, glm::vec2(m._dp) / (f32)num_steps / (f32)delta_time / (f32)ppm);
          else
            physics::set_velocity(a.body, glm::vec2(0, 0));
        }

        for (i32 i = 0; i < num_steps; i++) {
          // update space
          /* fixed timestep pas possible tant que pas d'interpolation entre les états
          physics::update_space(w.space, (f32)w.step_time);
          */
          physics::update_space(w.space, (f32)delta_time);
        }

        // update local pose of physically simulated actors from the physics wrapper
        dm.scale = glm::vec3(1,1,1);
        for (e = idlut::begin(w.actors); e < end; e++) {
          Actor &a = e->value;

          if (!a.dynamic || a.kinematic)
            continue;

          pose::get_local_pose(a.pose, dm);
          f32 rot_z = physics::get_body_rotation(a.body);
          //pose::set_local_rotation(a.pose, glm::quat(glm::vec3(dm.rotation.x, dm.rotation.y, rot_z)));
          dm.rotation = glm::quat(glm::vec3(dm.rotation.x, dm.rotation.y, rot_z));

          glm::vec3 position;
          physics::get_body_position(a.body, (glm::vec2&)position);
          position.x *= ppm;
          position.y *= ppm;
          position.z = dm.translation.z;

          //pose::set_local_translation(a.pose, position);
          dm.translation = position;

          compose_mat4(m, dm);
          pose::set_world_pose(a.pose, m);
          pose::update(a.pose);
        }

        { // update mover states
          MoverData md;
          for (me = idlut::begin(w.movers); me < mend; me++) {
            Mover &m = me->value;

            if (m._dirty) {
              Actor &a = *idlut::lookup(w.actors, m.capsule);
              md.mover = &m;
              physics::get_body_position(a.body, md.pos);
              md.pos *= ppm;

              m.collides_down  = false;
              m.collides_left  = false;
              m.collides_right = false;
              m.collides_up    = false;
              physics::get_body_contacts(a.body, update_mover_collision_state, &md);
              m._dirty = false;
            }
          }
        }

        { // update actor listeners
          const Hash<ActorListener>::Entry *l, *lend = hash::end(w.listeners);
          const Hash<ActorListener::PrevContact>::Entry *c, *cend;
          ContactData cd;
          cd.w = &w;

          for (l = hash::begin(w.listeners); l < lend; l++) {
            Actor &a = *idlut::lookup(w.actors, l->key);
            Array<ContactPoint> &result = *l->value.result;
            Array<ContactPoint> &contacts = *l->value.contacts;
            Hash<ActorListener::PrevContact> &prev_contacts = *l->value.prev_contacts;
            ActorListener::PrevContact pc ={ 0 };

            array::clear(contacts);
            array::clear(result);
            cd.data = &contacts;
            physics::get_body_contacts(a.body, push_actor_contact, &cd);

            cend = hash::end(prev_contacts);
            for (c = hash::begin(prev_contacts); c < cend; c++) {
              pc = c->value;
              pc.was_touching = false;
              hash::set(prev_contacts, c->key, pc);
            }

            for (u32 i = 0; i < array::size(contacts); i++) {
              if (!hash::has(prev_contacts, contacts[i].actor) && l->value.touched_callback)
                array::push_back(result, contacts[i]);
              pc = hash::get(prev_contacts, contacts[i].actor, pc);
              pc.contact = contacts[i];
              pc.was_touching = true;
              hash::set(prev_contacts, contacts[i].actor, pc);
            }

            if (l->value.touched_callback && array::size(result) > 0)
              l->value.touched_callback(result);

            array::clear(result);
            cend = hash::end(prev_contacts);
            for (c = hash::begin(prev_contacts); c < cend; c++) {
              if (!c->value.was_touching)
                array::push_back(result, c->value.contact);
            }

            if (l->value.untouched_callback && array::size(result) > 0)
              l->value.untouched_callback(result);

            for (u32 i = 0; i < array::size(result); i++)
              hash::remove(prev_contacts, result[i].actor);
          }
        }
      }
    }

    void finalize_update(PhysicsWorld &w)
    {
      update_wrapper_poses(w);
    }

    void set_gravity(PhysicsWorld &w, const glm::vec3 &v)
    {
      physics::set_gravity(w.space, v.x, v.y);
    }

    u64 create_raycast(PhysicsWorld &w, RaycastCallback callback, bool closest, bool any, const char *filter)
    {
      Allocator &a = *w.actors._data._allocator;
      Raycast rc;

      rc.callback = callback;
      rc.closest  = closest;
      rc.any      = any;

      if (filter) {
        const CollisionFilter &cf = get_collision_filter(murmur_hash_32(filter));
        rc.is = cf.is;
        rc.collides_with = cf.collides_with;
      } else {
        rc.is = physics::IS_ALL;
        rc.collides_with = physics::COLLIDES_WITH_ALL;
      }

      rc.hits = MAKE_NEW(a, Array<ContactPoint>, a);
      return idlut::add(w.raycasts, rc);
    }

    void cast_raycast(PhysicsWorld &w, u64 raycast, const glm::vec3 &from, const glm::vec3 &to)
    {
      Raycast *rc = idlut::lookup(w.raycasts, raycast);

      RaycastData rd;
      rd.hits  = rc->hits;
      rd.world = &w;

      physics::raycast(w.space, glm::vec2(from) / (f32)ppm, glm::vec2(to) / (f32)ppm, push_raycast_hit, rc->closest, rc->any, rc->is, rc->collides_with, &rd);

      const u32 num_hits = array::size(*rc->hits);
      if (rc->closest && num_hits > 0) {
        (*rc->hits)[0] = (*rc->hits)[num_hits-1];
        array::resize(*rc->hits, 1);
      }

      rc->callback(*(const Array<ContactPoint>*)rc->hits);
      array::clear(*rd.hits);
    }

    void destroy_raycast(PhysicsWorld &w, u64 raycast)
    {
      Allocator &a = *w.actors._data._allocator;
      Raycast *rc = idlut::lookup(w.raycasts, raycast);
      MAKE_DELETE(a, Array<ContactPoint>, rc->hits);
      idlut::remove(w.raycasts, raycast);
    }
  }

  PhysicsWorld::PhysicsWorld(f64 step_time, Allocator &a) :
    simulate(true),
    actors(a),
    actor_shapes(a),
    shape_infos(a),
    body_actor(a),
    raycasts(a),
    listeners(a),
    movers(a),
    step_time(step_time),
    space(physics::create_space()),
    accumulator(0) {};

  PhysicsWorld::~PhysicsWorld()
  {
    Allocator &a = *this->actors._data._allocator;

    { // deallocate raycasts memory
      const IdLookupTable<Raycast>::Entry *e, *end = idlut::end(this->raycasts);
      for (e = idlut::begin(this->raycasts); e < end; e++)
        MAKE_DELETE(a, Array<ContactPoint>, e->value.hits);
    }

    { // deallocate listener memory
      const Hash<ActorListener>::Entry *e, *end = hash::end(this->listeners);
      for (e = hash::begin(this->listeners); e < end; e++) {
        MAKE_DELETE(a, Array<ContactPoint>, e->value.result);
        MAKE_DELETE(a, Array<ContactPoint>, e->value.contacts);
        MAKE_DELETE(a, Hash<ActorListener::PrevContact>, e->value.prev_contacts);
      }
    }

    // destroy wrapper space
    physics::destroy_space(space);
  }
}