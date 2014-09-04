#include <Box2D/Box2D.h>

#include <stdlib.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <runtime/trace.h>
#include <runtime/memory.h>
#include <runtime/assert.h>
#include <runtime/array.h>
#include <physics/physics_wrapper.h>

// http://www.box2d.org/forum/viewtopic.php?f=3&t=8834&view=next

namespace
{
  using namespace pge;

  class PgContactListener : public b2ContactListener
  {
  public:
    // void BeginContact(b2Contact* contact) { }
    // void EndContact(b2Contact* contact) { }
    void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) {
      (void)oldManifold;
      b2Fixture* fixtureA = contact->GetFixtureA();
      b2Fixture* fixtureB = contact->GetFixtureB();

      //check if one of the fixtures is the platform
      b2Fixture* platformFixture = NULL;
      b2Fixture* otherFixture = NULL;
      if (fixtureA->GetBody()->GetType() == b2BodyType::b2_staticBody) {
        platformFixture = fixtureA;
        otherFixture = fixtureB;
      } else if (fixtureB->GetBody()->GetType() == b2BodyType::b2_staticBody) {
        platformFixture = fixtureB;
        otherFixture = fixtureA;
      }

      if (!platformFixture)
        return;

      b2Body* otherBody = otherFixture->GetBody();
      b2Body* platformBody = platformFixture->GetBody();
      int numPoints = contact->GetManifold()->pointCount;
      b2WorldManifold worldManifold;
      contact->GetWorldManifold(&worldManifold);

      //check if contact points are moving downward
      for (int i = 0; i < numPoints; i++) {

        b2Vec2 pointVelPlatform =
          platformBody->GetLinearVelocityFromWorldPoint(worldManifold.points[i]);
        b2Vec2 pointVelOther =
          otherBody->GetLinearVelocityFromWorldPoint(worldManifold.points[i]);
        b2Vec2 relativeVel = platformBody->GetLocalVector(pointVelOther - pointVelPlatform);

        if (relativeVel.y < -1) //if moving down faster than 1 m/s, handle as before
          return;//point is moving into platform, leave contact solid and exit
        else if (relativeVel.y < 1) { //if moving slower than 1 m/s
          //borderline case, moving only slightly out of platform
          b2Vec2 relativePoint = platformBody->GetLocalPoint(worldManifold.points[i]);
          float platformFaceY = 0.0f; //front of platform, from fixture definition :(
          if (relativePoint.y > platformFaceY - 0.05)
            return;//contact point is less than 5cm inside front face of platfrom
        } else
          ;//moving up faster than 1 m/s
      }

      //no points are moving downward, contact should not be solid
      contact->SetEnabled(false);
    }
    //void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) {}
  };

  static i32 global_velocity_iterations;
  static i32 global_position_iterations;
  static PgContactListener global_contact_listener;

  inline b2World *to_b2World(const Space *s)
  {
    return (b2World*)s;
  }

  inline b2Shape *to_b2Shape(const Shape *s)
  {
    return (b2Shape*)s;
  }

  inline b2Fixture *to_b2Fixture(const Fixture *s)
  {
    return (b2Fixture*)s;
  }

  inline b2Body *to_b2Body(const Body* b)
  {
    return (b2Body*)(b);
  }

  inline b2Vec2 to_b2Vec2(const glm::vec2 &v)
  {
    return b2Vec2(v.x, v.y);
  }

  inline glm::vec2 to_vec2(const b2Vec2 &v)
  {
    return glm::vec2(v.x, v.y);
  }


  void move_polygon_shape(b2PolygonShape* polygon, b2Vec2 move)
  {
    for (int32 i=0; i < polygon->GetVertexCount(); i++) {
      polygon->m_vertices[i] += move;
    }
    polygon->m_centroid += move;
  }
}


namespace pge
{
  using namespace glm;

  namespace physics
  {
    void initialize(i32 velocity_iterations, i32 position_iterations)
    {
      global_velocity_iterations = velocity_iterations;
      global_position_iterations = position_iterations;
    }


    // SPACE

    Space *create_space(void)
    {
      b2Vec2 gravity(0.f, -10.f);
      b2World *s = MAKE_NEW(memory_globals::default_allocator(), b2World, gravity);
      //s->SetContactListener(&global_contact_listener);
      return s;
    }

    Space *create_space(f32 gravity_x, f32 gravity_y)
    {
      b2Vec2 gravity(gravity_x, gravity_y);
      b2World *s = MAKE_NEW(memory_globals::default_allocator(), b2World, gravity);
      //s->SetContactListener(&global_contact_listener);
      return s;
    }

    void destroy_space(Space *space)
    {
      MAKE_DELETE(memory_globals::default_allocator(), b2World, to_b2World(space));
    }

    void update_space(Space *space, f32 delta_time)
    {
      to_b2World(space)->Step(delta_time, global_velocity_iterations, global_position_iterations);
    }

    void set_gravity(Space *space, f32 x, f32 y)
    {
      to_b2World(space)->SetGravity(b2Vec2(x, y));
    }


    // BODY
    Body *create_body(Space *space, f32 offset_x, f32 offset_y, BodyType type)
    {
      b2BodyDef body;
      body.type = (b2BodyType)type;
      body.position.Set(offset_x, offset_y);
      return to_b2World(space)->CreateBody(&body);
    }

    void destroy_body(Body *body)
    {
      b2Body *b = to_b2Body(body);
      b->GetWorld()->DestroyBody(b);
    }

    void set_body_type(Body *body, BodyType type)
    {
      b2Body *b = to_b2Body(body);
      b->SetType((b2BodyType)type);
    }

    void get_body_position(const Body *body, glm::vec2 &v)
    {
      v = to_vec2(to_b2Body(body)->GetPosition());
    }

    void set_body_position(Body *body, const glm::vec2 &pos)
    {
      b2Body *b = to_b2Body(body);
      b->SetTransform(to_b2Vec2(pos), b->GetAngle());
    }

    f32 get_body_rotation(const Body *body)
    {
      return to_b2Body(body)->GetAngle();
    }

    void set_body_rotation(Body *body, f32 rotation)
    {
      b2Body *b = to_b2Body(body);
      b->SetTransform(b->GetPosition(), rotation);
    }

    bool is_sleeping(const Body *body)
    {
      return !to_b2Body(body)->IsAwake();
    }

    void set_awake(Body *body, bool v)
    {
      to_b2Body(body)->SetAwake(v);
    }

    void get_velocity(const Body *body, glm::vec2 &v)
    {
      v = to_vec2(to_b2Body(body)->GetLinearVelocity());
    }

    void set_velocity(Body *body, const glm::vec2 &v)
    {
      to_b2Body(body)->SetLinearVelocity(to_b2Vec2(v));
    }


    void enable_gravity(Body *body)
    {
      to_b2Body(body)->SetGravityScale(1.f);
    }

    void disable_gravity(Body *body)
    {
      to_b2Body(body)->SetGravityScale(0.f);
    }

    void set_body_filter(Body *body, u16 is, u16 collides_with)
    {
      b2Body *b    = to_b2Body(body);
      b2Fixture *f = b->GetFixtureList();

      while (f != NULL) {
        set_filter(f, is, collides_with);
        f = f->GetNext();
      }
    }

    void apply_impulse(Body *body, const glm::vec2 &impulse)
    {
      b2Body *b = to_b2Body(body);
      b->ApplyLinearImpulse(to_b2Vec2(impulse), b->GetWorldCenter(), true);
    }


    // SHAPE

    Shape *create_circle(f32 radius, const glm::vec2 &offset)
    {
      b2CircleShape *circle = MAKE_NEW(memory_globals::default_allocator(), b2CircleShape);
      circle->m_p      = to_b2Vec2(offset);
      circle->m_radius = radius;
      return circle;
    }

    Shape *create_chain(const glm::vec2 *vertices, i32 num_verts)
    {
      b2ChainShape *chain = MAKE_NEW(memory_globals::default_allocator(), b2ChainShape);
      chain->CreateChain((b2Vec2*)vertices, num_verts);
      return chain;
    }

    Shape *create_box(f32 width, f32 height)
    {
      b2PolygonShape *box = MAKE_NEW(memory_globals::default_allocator(), b2PolygonShape);
      box->SetAsBox(width, height);
      return box;
    }

    Shape *create_polygon(const glm::vec2 *vertices, i32 num_verts)
    {
      b2PolygonShape *poly = MAKE_NEW(memory_globals::default_allocator(), b2PolygonShape);
      poly->Set((b2Vec2*)vertices, num_verts);
      return poly;
    }

    void destroy_shape(Shape *s)
    {
      b2Shape *shape = to_b2Shape(s);
      Allocator &a   = memory_globals::default_allocator();

      switch (shape->m_type) {
        case b2Shape::e_chain:
          MAKE_DELETE(a, b2ChainShape, ((b2ChainShape*)s));
          break;
        case b2Shape::e_circle:
          MAKE_DELETE(a, b2CircleShape, ((b2CircleShape*)s));
          break;
        case b2Shape::e_edge:
          MAKE_DELETE(a, b2EdgeShape, (b2EdgeShape*)s);
          break;
        case b2Shape::e_polygon:
          MAKE_DELETE(a, b2PolygonShape, ((b2PolygonShape*)s));
          break;
        default:
          XERROR("Destroy_shape not handled, type: ", shape->m_type);
          break;
      }
    }

    void get_shape_position(const Shape *s, glm::vec2 &p)
    {
      b2Shape *shape = to_b2Shape(s);

      switch (shape->m_type) {
        case b2Shape::e_circle:
          p = to_vec2(static_cast<const b2CircleShape*>(shape)->m_p);
          break;
        case b2Shape::e_polygon:
          p = to_vec2(static_cast<const b2PolygonShape*>(shape)->m_centroid);
          break;
        default:
          XERROR("Get position not allowed on shape type: ", shape->m_type);
          break;
      }
    }

    void set_shape_position(Shape *s, const glm::vec2 &pos)
    {
      b2Shape *shape = to_b2Shape(s);

      switch (shape->m_type) {
        case b2Shape::e_circle:
          ((b2CircleShape*)shape)->m_p = to_b2Vec2(pos);
          break;
        case b2Shape::e_polygon:
        {
          b2PolygonShape* polygon = (b2PolygonShape*)(shape);
          move_polygon_shape(polygon, to_b2Vec2(pos) - polygon->m_centroid);
        }
          break;
        default:
          XERROR("Set position not allowed on shape type: ", shape->m_type);
      }
    }


    f32 get_circle_radius(const Shape *circle)
    {
      return ((b2CircleShape*)circle)->m_radius;
    }

    const glm::vec2 *get_vertices(const Shape *shape)
    {
      switch (((b2Shape*)shape)->m_type) {
        case b2Shape::Type::e_chain:
          return (glm::vec2*)((b2ChainShape*)shape)->m_vertices;
        case b2Shape::Type::e_polygon:
          return(glm::vec2*)((b2PolygonShape*)shape)->m_vertices;
      }
      XERROR("Function not allowed for shape type: %d", ((b2Shape*)shape)->m_type);
      return NULL;
    }

    i32 get_num_vertices(const Shape *shape)
    {
      switch (((b2Shape*)shape)->m_type) {
        case b2Shape::Type::e_chain:
          return ((b2ChainShape*)shape)->m_count;
        case b2Shape::Type::e_polygon:
          return ((b2PolygonShape*)shape)->m_count;
      }
      XERROR("Function not allowed for shape type: %d", ((b2Shape*)shape)->m_type);
      return 0;
    }


    // FIXTURE

    Fixture *create_fixture(Body *body, Shape *shape, f32 density, f32 friction, f32 restitution)
    {
      b2Fixture *f = to_b2Body(body)->CreateFixture(to_b2Shape(shape), density);
      f->SetFriction(friction);
      f->SetRestitution(restitution);
      return f;
    }

    Fixture *create_fixture(Body *body, Shape *shape, Fixture *other)
    {
      b2Fixture *o = to_b2Fixture(other);
      b2Fixture *f = to_b2Body(body)->CreateFixture(to_b2Shape(shape), o->GetDensity());
      f->SetFriction(o->GetFriction());
      f->SetRestitution(o->GetRestitution());
      f->SetFilterData(o->GetFilterData());
      f->SetUserData(o->GetUserData());
      f->SetSensor(o->IsSensor());
      return f;
    }

    void destroy_fixture(Body *body, Fixture *fixture)
    {
      to_b2Body(body)->DestroyFixture(to_b2Fixture(fixture));
    }

    void set_fixture_position(Fixture *f, const glm::vec2 &pos)
    {
      b2Fixture *fixture = to_b2Fixture(f);
      set_shape_position(fixture->GetShape(), pos);

      //It is necessary to recalculate AABB box of fixture
      //Because it is not automatic if body is static.
      bool active = fixture->GetBody()->IsActive();
      fixture->GetBody()->SetActive(!active);
      fixture->GetBody()->SetActive(active);
    }

    void get_fixture_position(const Fixture *f, glm::vec2 &p)
    {
      get_shape_position((Shape*)to_b2Fixture(f)->GetShape(), p);
    }

    void get_fixture_world_position(const Fixture *f, glm::vec2 &p)
    {
      b2Fixture *fixture = to_b2Fixture(f);
      get_shape_position(fixture->GetShape(), p);
      p += to_vec2(fixture->GetBody()->GetPosition());
    }


    void set_density(Fixture *f, f32 value)
    {
      b2Fixture *fixture = to_b2Fixture(f);
      fixture->SetDensity(value);
      fixture->GetBody()->ResetMassData();
    }

    void set_friction(Fixture *f, f32 value)
    {
      to_b2Fixture(f)->SetFriction(value);
    }

    void set_restitution(Fixture *f, f32 value)
    {
      to_b2Fixture(f)->SetRestitution(value);
    }


    void set_filter(Fixture *fixture, u16 is, u16 collides_with)
    {
      b2Filter filter;
      filter.categoryBits = is;
      filter.maskBits     = collides_with;
      to_b2Fixture(fixture)->SetFilterData(filter);
    }

    void set_group(Fixture *fixture, u32 group)
    {
      b2Filter filter = to_b2Fixture(fixture)->GetFilterData();
      filter.groupIndex = (i16)group;
      to_b2Fixture(fixture)->SetFilterData(filter);
    }

    void set_trigger(Fixture *fixture, bool value)
    {
      to_b2Fixture(fixture)->SetSensor(value);
    }

    // bounding box    

    void get_body_contacts(Body *body, WrapperContactCallback callback, void *data)
    {
      b2WorldManifold wm;

      for (b2ContactEdge* ce = to_b2Body(body)->GetContactList(); ce; ce = ce->next) {
        if (!ce->contact->IsTouching() || !ce->contact->IsEnabled())
          continue;

        ce->contact->GetWorldManifold(&wm);
        callback(glm::vec2(wm.points[0].x, wm.points[0].y), wm.separations[0], to_vec2(wm.normal), ce->other, data);
      }
    }

    class MyRayCastCallback : public b2RayCastCallback
    {
    public:
      MyRayCastCallback(WrapperContactCallback callback, void *data, const b2Filter &filter, bool closest, bool any, f32 ray_length) :
        _callback(callback), _data(data), _filter(filter), _closest(closest), _any(any), _ray_length(ray_length) {}

      f32 ReportFixture(b2Fixture *fixture, const b2Vec2 &position, const b2Vec2 &normal, f32 fraction)
      {
        const b2Filter &f = fixture->GetFilterData();

        // skip fixtures that do not match filter
        if (!((_filter.maskBits & f.categoryBits) != 0 && (_filter.categoryBits & f.maskBits) != 0))
          return -1;

        _callback(to_vec2(position), fraction * _ray_length, to_vec2(normal), fixture->GetBody(), _data);

        // if any is true, stop the reporting since we have dounf a hit
        if (_any)
          return 0;

        // if closest is true, return the fraction as is to be sure the last hit added is the closest
        return _closest ? fraction : 1;
      }

    private:
      void(*_callback)(const glm::vec2 &position, f32 distance, const glm::vec2 &normal, const Shape *shape, void *data);
      void *_data;
      bool  _closest;
      bool  _any;
      b2Fixture *_fixture;
      b2Filter   _filter;
      f32        _ray_length;
    };

    // query
    void raycast(Space *space, const glm::vec2 &from, const glm::vec2 &to, WrapperContactCallback callback,
                 bool closest, bool any, u16 is, u16 collides_with, void *data)
    {
      b2Filter filter;
      filter.categoryBits = is;
      filter.maskBits = collides_with;
      MyRayCastCallback c(callback, data, filter, closest, any, glm::distance(from, to));
      to_b2World(space)->RayCast(&c, to_b2Vec2(from), to_b2Vec2(to));
    }
  }
}
