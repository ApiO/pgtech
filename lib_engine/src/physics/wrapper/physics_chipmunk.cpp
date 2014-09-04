#include <stdlib.h>
#include <chipmunk.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <runtime/trace.h>
#include <runtime/memory.h>
#include <runtime/assert.h>
#include <runtime/array.h>
#include <physics/physics_wrapper.h>

//http://chipmunk-physics.net/release/ChipmunkLatest-Docs/

#define vec2_to_cpVect(v) (cpv((v).x, (v).y))
#define cpVect_to_vec2(v) (glm::vec2((v).x, (v).y))

#define _body(b)  ((cpBody*)(b))
#define _shape(s) ((cpShape*)(s))
#define _space(s) ((cpSpace*)(s))

#define _destroy_shape(shape)                \
  MULTI_LINE_MACRO_BEGIN                     \
  cpSpace *space = cpShapeGetSpace((shape)); \
  if (space)                                 \
  cpSpaceRemoveShape(space, (shape));        \
  cpShapeFree((shape));                      \
  MULTI_LINE_MACRO_END


namespace
{
  using namespace pge;
  static void _shape_destroy_iterator(cpBody *Body, cpShape *shape, void *data)
  {
    (void)Body, data;
    _destroy_shape(shape);
  }
}

namespace pge
{
  namespace physics
  {
    using namespace glm;

    void _initialize()
    {
      cpEnableSegmentToSegmentCollisions();
    }

    //  - Space

    Space *create_space(void)
    {
      return cpSpaceNew();
    }

    Space *create_space(f32 x, f32 y)
    {
      cpSpace *space = cpSpaceNew();
      cpSpaceSetGravity(space, cpv(x, y));
      return space;
    }

    void destroy_space(Space *space)
    {
      cpSpaceFree(_space(space));
      space = NULL;
    }

    void update_space(Space *space, f32 delta_time)
    {
      cpSpaceStep(_space(space), delta_time);
    }

    void set_gravity(Space *space, f32 x, f32 y)
    {
      cpSpaceSetGravity(_space(space), cpv(x, y));
    }

    void add_shape(Space *space, Shape *shape)
    {
      cpSpaceAddShape(_space(space), _shape(shape));
    }

    //  - Body

    Body *create_new_static(Space *space)
    {
      Body *body = cpBodyNewStatic();
      cpSpaceReindexStatic(_space(space));
      return body;
    }

    Body *create_new_body(f32 mass, f32 inertia)
    {
      return cpBodyNew(mass, inertia);
    }

    Body *create_new_body()
    {
      return cpBodyNew(INFINITY, INFINITY);
    }

    void set_mass_and_moment(Body *body, f32 mass, f32 moment)
    {
      cpBodySetMass(_body(body), mass);
      cpBodySetMoment(_body(body), moment);
    }

    void add_body(Space *space, Body *body)
    {
      cpSpaceAddBody(_space(space), _body(body));
    }

    glm::vec2 get_position(const Body *body)
    {
      return cpVect_to_vec2(cpBodyGetPos(_body(body)));
    }

    void set_position(Space *space, Body *b, const glm::vec2 &pos)
    {
      cpBody *body = _body(b);
      cpBodySetPos(body, vec2_to_cpVect(pos));

      cpSpaceReindexShapesForBody(_space(space), body);
    }

    f32 get_rotation(const Body *body)
    {
      return (f32)cpBodyGetAngle(_body(body));
    }

    void set_rotation(Space *space, Body *b, f32 rotation)
    {
      cpBody *body = _body(b);
      cpBodySetAngle(body, rotation);

      cpSpaceReindexShapesForBody(_space(space), body);
    }

    bool is_sleeping(const Body *body)
    {
      return cpBodyIsSleeping(_body(body)) == cpTrue;
    }

    glm::vec2 get_velocity(const Body *body)
    {
      return cpVect_to_vec2(cpBodyGetForce(_body(body)));
    }

    void set_velocity(Body *body, const glm::vec2 &v)
    {
      cpBodySetForce(_body(body), vec2_to_cpVect(v));
    }

    void destroy_body(Body *body)
    {
      cpBody *b = _body(body);

      cpBodyEachShape(b, _shape_destroy_iterator, NULL);
      //cpBodyEachConstraint(b, _constraint_destroy_iterator, NULL);

      if (!(cpBodyIsRogue(b) || cpBodyIsStatic(b)))
        cpSpaceRemoveBody(cpBodyGetSpace(b), b);

      cpBodyFree(b);
    }

    //  - Shape

    void set_local_position(Shape *shape, const glm::vec2 &pos, const glm::vec2 &rot)
    {
      cpShapeUpdate(_shape(shape), cpv(pos.x, pos.y), cpv(rot.x, rot.y));
    }

    void reset_local_position(Shape *shape)
    {
      cpShapeCacheBB(_shape(shape));
    }

    void set_friction(Shape *shape, f32 value)
    {
      cpShapeSetFriction(_shape(shape), value);
    }

    void set_elasticity(Shape *shape, f32 value)
    {
      cpShapeSetElasticity(_shape(shape), value);
    }

    Body *get_shape_body(const Shape *shape)
    {
      return cpShapeGetBody(_shape(shape));
    }

    mat4 get_body_pose(const Shape *shape)
    {
      mat4 result(1.f);
      vec2 translation = cpVect_to_vec2(cpBodyGetPos(cpShapeGetBody(_shape(shape))));
      f32  rotation    = degrees((f32)cpBodyGetAngle(cpShapeGetBody(_shape(shape))));

      result = translate(result, vec3(translation, 0.f));
      quat q = angleAxis(rotation, vec3(0.f, 0.f, 1.f));
      result = result * toMat4(q);
      return result;
    }

    vec2 get_body_position(const Shape *shape)
    {
      return cpVect_to_vec2(cpBodyGetPos(cpShapeGetBody(_shape(shape))));
    }

    f32 get_body_rotation(const Shape *shape)
    {
      return (f32)cpBodyGetAngle(cpShapeGetBody(_shape(shape)));
    }

    void destroy_shape(Shape *shape)
    {
      cpShape *cps = _shape(shape);
      _destroy_shape(cps);
    }

    Shape *create_circle(const Body *body, f32 radius, const glm::vec2 &offset)
    {
      cpShape *shape = cpCircleShapeNew(_body(body), radius, vec2_to_cpVect(offset));
      cpResetShapeIdCounter();
      return shape;
    }

    glm::vec2 get_offset(const Shape *circle)
    {
      return cpVect_to_vec2(cpCircleShapeGetOffset(_shape(circle)));
    }

    f32 get_circle_radius(const Shape *circle)
    {
      return cpCircleShapeGetRadius(_shape(circle));
    }


    //  - segment
    Shape *create_segment(const Body *body, const glm::vec2 &a, const glm::vec2 &b)
    {
      cpShape *shape = cpSegmentShapeNew(_body(body), vec2_to_cpVect(a), vec2_to_cpVect(b), 0.f);
      cpResetShapeIdCounter();
      return shape;
    }

    glm::vec2 get_A(const Shape *segment)
    {
      return cpVect_to_vec2(cpSegmentShapeGetA(_shape(segment)));
    }

    glm::vec2 get_B(const Shape *segment)
    {
      return cpVect_to_vec2(cpSegmentShapeGetB(_shape(segment)));
    }

    glm::vec2 get_normal(const Shape *segment)
    {
      return cpVect_to_vec2(cpSegmentShapeGetNormal(_shape(segment)));
    }

    f32 get_segment_radius(const Shape *segment)
    {
      return cpSegmentShapeGetRadius(_shape(segment));
    }

    /*
    When you have a number of segment shapes that are all joined together,
    things can still collide with the “cracks” between the segments.
    By setting the neighbor segment endpoints you can tell Chipmunk to avoid
    colliding with the inner parts of the crack.
    */
    void set_neighbors(Shape *segment, const glm::vec2 prev, const glm::vec2 next)
    {
      cpSegmentShapeSetNeighbors(_shape(segment), vec2_to_cpVect(prev), vec2_to_cpVect(next));
    }



    //  - box

    Shape *create_box(const Body *body, f32 width, f32 height)
    {
      cpShape *shape = cpBoxShapeNew(_body(body), width, height);
      cpResetShapeIdCounter();

      return shape;
    }


    //  - polygon

    Shape *create_polygon(const Body *body, const glm::vec2 *vertices, i32 num_verts)
    {
      cpShape *shape = cpPolyShapeNew(_body(body), num_verts, (const cpVect*)vertices, cpv(0.f, 0.f));
      cpResetShapeIdCounter();

      return shape;
    }

    i32 get_num_vertices(const Shape *polygon)
    {
      return cpPolyShapeGetNumVerts(_shape(polygon));
    }

    glm::vec2 get_vertices(const Shape *polygon, i32 index)
    {
      return cpVect_to_vec2(cpPolyShapeGetVert(_shape(polygon), index));
    }

    bool validate(const glm::vec2 *vertices, u32 num_verts)
    {
      return cpPolyValidate((cpVect*)vertices, num_verts) == 1;
    }

    glm::vec2 centroid(const glm::vec2 *vertices, u32 num_verts)
    {
      return cpVect_to_vec2(cpCentroidForPoly(num_verts, (cpVect*)vertices));
    }

    void recenter(glm::vec2 *vertices, u32 num_verts)
    {
      cpRecenterPoly(num_verts, (cpVect*)vertices);
    }




    // filter / group

    void set_group(Shape *shape, u32 group)
    {
      cpShapeSetGroup(_shape(shape), group);
    }

    void set_filter(Shape *shape, u16 is, u16 collides_with)
    {
      cpLayers l;
      l.is = is;
      l.collides_with = collides_with;
      cpShapeSetLayers(_shape(shape), l);
    }

    void set_trigger(Shape *shape, bool value)
    {
      cpShapeSetSensor(_shape(shape), value);
    }


    // bounding box
    /*
    void destroy(Constraint &constraint)
    {
    cpConstraint *cst = _constraint(constraint);
    destroy_constraint(cst);
    constraint.data = NULL;
    }
    */


    i32 convex_hull(const glm::vec2 *verts, i32 num_vertices, glm::vec2 *result, i32 *fisrt, f32 tol)
    {
      return cpConvexHull(num_vertices, (cpVect*)verts, (cpVect*)result, fisrt, tol);
    }

    f32 moment_for_circle(f32 mass, f32 diameter1, f32 diameter2)
    {
      return cpMomentForCircle(mass, diameter1, diameter2, cpv(0.f, 0.f));
    }

    f32 moment_for_segment(f32 mass, glm::vec2 &a, glm::vec2 &b)
    {
      return cpMomentForSegment(mass, vec2_to_cpVect(a), vec2_to_cpVect(b));
    }

    f32 moment_for_poly(f32 mass, glm::vec2 *points, i32 num_points)
    {
      return cpMomentForPoly(mass, num_points, (cpVect*)points, cpv(0.f, 0.f));
    }

    f32 moment_for_box(f32 mass, f32 width, f32 height)
    {
      return cpMomentForBox(mass, width, height);
    }

    f32 area_for_circle(const Shape *shape)
    {
      return (f32)cpAreaForCircle(cpCircleShapeGetRadius(_shape(shape)), 0.f);
    }

    f32 area_for_segment(const Shape *shape)
    {
      return (f32)cpAreaForSegment(cpSegmentShapeGetA(_shape(shape)), cpSegmentShapeGetB(_shape(shape)), 0.f);
    }

    f32 area_for_poly(const Shape *shape)
    {
      cpVect v = cpPolyShapeGetVert(_shape(shape), 0);
      return (f32)cpAreaForPoly(cpPolyShapeGetNumVerts(_shape(shape)), &v);
    }

    f32 area_for_box(const Shape *shape)
    {
      cpVect v = cpPolyShapeGetVert(_shape(shape), 0);
      return (f32)cpAreaForPoly(cpPolyShapeGetNumVerts(_shape(shape)), &v);
    }

    struct RaycastData {
      const glm::vec2 *from;
      const glm::vec2 *to;
      void *callback_data;
      void(*callback)(const glm::vec2 &position, f32 distance, const glm::vec2 &normal, const Shape *shape, void *data);
    };

    static void raycast_callback(cpShape *shape, cpFloat t, cpVect n, void *data)
    {
      RaycastData &rd = *(RaycastData*)data;
      glm::vec2 position, normal;

      position = *rd.from + (*rd.to - *rd.from)*t;
      f32 distance = glm::distance(*rd.from, position);
      normal = cpVect_to_vec2(n);

      rd.callback(position, distance, normal, shape, rd.callback_data);
    }

    // query
    void raycast(Space *space,
                 const glm::vec2 &from,
                 const glm::vec2 &to,
                 void(*callback)(const glm::vec2 &position, f32 distance, const glm::vec2 &normal, const Shape *shape, void *data),
                 bool closest,
                 u16  is,
                 u16  collides_with,
                 void *data)
    {
      cpLayers f;
      RaycastData rd;
      rd.from = &from;
      rd.to   = &to;
      rd.callback = callback;
      rd.callback_data = data;

      f.is = is;
      f.collides_with = collides_with;

      if (closest) {
        cpSegmentQueryInfo ri;
        cpShape *shape = cpSpaceSegmentQueryFirst(_space(space),
                                                  vec2_to_cpVect(from),
                                                  vec2_to_cpVect(to), f, CP_NO_GROUP, &ri);

        if (shape)
          raycast_callback(shape, ri.t, ri.n, &rd);
        return;
      }

      cpSpaceSegmentQuery(_space(space),
                          vec2_to_cpVect(from),
                          vec2_to_cpVect(to), f, CP_NO_GROUP, raycast_callback, &rd);
    }

    struct ContactData
    {
      void(*callback)(const glm::vec2 &position, f32 distance, const glm::vec2 &normal, const Body *body, void *data);
      void *data;
    };

    static void contact_callback(cpBody *body, cpArbiter *arbiter, void *data)
    {
      (void)body;
      ContactData &cd = *(ContactData*)data;
      CP_ARBITER_GET_BODIES(arbiter, b, other);
      cpContactPointSet cps = cpArbiterGetContactPointSet(arbiter);
      cd.callback(cpVect_to_vec2(cps.points[0].point),
                  cps.points[0].dist,
                  cpVect_to_vec2(cps.points[0].normal),
                  other, cd.data);
    }

    void get_body_contacts(Body *body, void(*callback)(const glm::vec2 &position, f32 distance, const glm::vec2 &normal, const Body *body, void *data), void *data)
    {
      ContactData cd;
      cd.callback = callback;
      cd.data = data;
      cpBodyEachArbiter(_body(body), contact_callback, &cd);
    }

    struct AABBData
    {
      void(*callback)(const Shape *shape, void *data);
      void *data;
    };

    static void aabb_callback(cpShape *shape, void *data)
    {
      AABBData &ad = *(AABBData*)data;
      ad.callback((Shape*)shape, ad.data);
    }

    void aabb_query(Space *space, const glm::vec2 &aabb, const glm::vec2 &position,
                    void(*callback)(const Shape *shape, void *data), u16 is, u16 collides_with, void *data)
    {
      AABBData ad      ={ callback, data };
      const cpLayers f ={ is, collides_with };

      cpBB cpbb = cpBBNew(position.x - aabb.x/2, position.y - aabb.y/2, position.x + aabb.x/2, position.y + aabb.y/2);
      ad.callback = callback;
      ad.data = data;
      cpSpaceBBQuery(_space(space), cpbb, f, CP_NO_GROUP, aabb_callback, &ad);
    }

    static void shape_query_callback(cpShape *shape, cpContactPointSet *cps, void *data)
    {
      ContactData &cd = *(ContactData*)data;
      cd.callback(cpVect_to_vec2(cps->points[0].point), cps->points[0].dist, cpVect_to_vec2(cps->points[0].normal), shape, cd.data);
    }

    void shape_query(Space *space, Shape *shape, const glm::vec2 &position, f32 rotation,
                     void(*callback)(const glm::vec2 &position, f32 distance, const glm::vec2 &normal, const Shape *shape, void *data),
                     u16 is, u16 collides_with, void *data)
    {
      ContactData cd   ={ callback, data };
      const cpLayers f ={ is, collides_with };

      cd.callback = callback;
      cd.data = data;

      cpShapeSetLayers(_shape(shape), f);
      cpShapeUpdate(_shape(shape), vec2_to_cpVect(position), vec2_to_cpVect(glm::vec2(rotation)));
      cpSpaceShapeQuery(_space(space), _shape(shape), shape_query_callback, &cd);
    }
  }
}