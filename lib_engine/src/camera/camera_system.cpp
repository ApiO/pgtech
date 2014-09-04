#include "camera_system.h"
#include <runtime/idlut.h>

namespace
{
  using namespace pge;

  inline void normalize_plane(Plane & plane)
  {
    f32 nf = 1.0f / sqrtf(plane.a * plane.a + plane.b * plane.b + plane.c * plane.c);
    plane.a = plane.a * nf;
    plane.b = plane.b * nf;
    plane.c = plane.c * nf;
    plane.d = plane.d * nf;
  }

  // EXPERIMENTAL from, http://web.archive.org/web/20120531231005/http://crazyjoke.free.fr/doc/3D/plane%20extraction.pdf, p.9
  // glm::mat4[] are overloaded to be row major, so this is the directX implementation
  void extract_frustum(const glm::mat4 &pv, Frustum &frustum)
  {
    // Left clipping plane
    frustum.left.a = pv[0][3] + pv[0][0];
    frustum.left.b = pv[1][3] + pv[1][0];
    frustum.left.c = pv[2][3] + pv[2][0];
    frustum.left.d = pv[3][3] + pv[3][0];

    // Right clipping plane
    frustum.right.a = pv[0][3] - pv[0][0];
    frustum.right.b = pv[1][3] - pv[1][0];
    frustum.right.c = pv[2][3] - pv[2][0];
    frustum.right.d = pv[3][3] - pv[3][0];

    // Top clipping plane
    frustum.top.a = pv[0][3] - pv[0][1];
    frustum.top.b = pv[1][3] - pv[1][1];
    frustum.top.c = pv[2][3] - pv[2][1];
    frustum.top.d = pv[3][3] - pv[3][1];

    // Bottom clipping plane
    frustum.bottom.a = pv[0][3] + pv[0][1];
    frustum.bottom.b = pv[1][3] + pv[1][1];
    frustum.bottom.c = pv[2][3] + pv[2][1];
    frustum.bottom.d = pv[3][3] + pv[3][1];

    // Near clipping plane
    frustum._near.a = pv[0][3] + pv[0][2];
    frustum._near.b = pv[1][3] + pv[1][2];
    frustum._near.c = pv[2][3] + pv[2][2];
    frustum._near.d = pv[3][3] + pv[3][2];

    // Far clipping plane
    frustum._far.a = pv[0][3] - pv[0][2];
    frustum._far.b = pv[1][3] - pv[1][2];
    frustum._far.c = pv[2][3] - pv[2][2];
    frustum._far.d = pv[3][3] - pv[3][2];

    // Normalizes
    Plane *p = (Plane*)&frustum;
    for (pge::u32 i = 0; i < 6; i++)
      normalize_plane(p[i]);
  }
}

namespace pge
{
  Camera::Camera() : update_projection(true),
    projection(1.f), view(1.f), projection_view(1.f){}

  CameraSystem::CameraSystem(Allocator &a) : IdLookupTable<Camera>(a){}
  CameraSystem::~CameraSystem(){}

  namespace camera_system
  {
    void update(CameraSystem &sys)
    {
      CameraSystem::Entry *c, *cend = idlut::end(sys);
      for (c = idlut::begin(sys); c < cend; c++) {
        Camera &camera = c->value;
        bool pose_dirty = pose::is_dirty(camera.pose);

        pose::update(camera.pose);

        // updates pose & view
        if (pose_dirty) {
          pose::get_world_pose(camera.pose, camera.view);
          camera.view = glm::inverse(camera.view);
        }

        // updates projection
        if (camera.update_projection) {
          switch (camera.projection_type) {
            case PROJECTION_PERSPECTIVE:
              camera.projection = glm::perspective(camera.fov, camera.aspect, camera.near_range, camera.far_range);
              break;
            case PROJECTION_ORTHOGRAPHIC:
              camera.projection = glm::ortho(camera.left, camera.right, camera.bottom, camera.top, camera.near_range, camera.far_range);
              break;
          }
        }

        if (pose_dirty || camera.update_projection) {
          camera.projection_view = camera.projection * camera.view;
          camera.update_projection = false;
          extract_frustum(camera.projection_view, camera.frustum);
        }
      }
    }
  }
}