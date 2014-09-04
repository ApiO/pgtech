#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <math.h>

#include <runtime/assert.h>
#include "matrix_types.h"

namespace pge
{
  void compose_mat4(glm::mat4 &m, const DecomposedMatrix &dm);
  void decompose_mat4(const glm::mat4 &m, DecomposedMatrix &dm);
  void get_mat4_translation(const glm::mat4 &m, glm::vec3 &result);
}

namespace pge
{
  namespace matrix_internal
  {
    enum UnmatrixIndices {
      U_SCALEX = 0,
      U_SCALEY,
      U_SCALEZ,
      U_ROTATEX,
      U_ROTATEY,
      U_ROTATEZ,
      U_TRANSX,
      U_TRANSY,
      U_TRANSZ
    };

    enum UnmatrixFullIndices {
      U_FULL_SCALEX = 0,
      U_FULL_SCALEY,
      U_FULL_SCALEZ,
      U_FULL_SHEARXY,
      U_FULL_SHEARXZ,
      U_FULL_SHEARYZ,
      U_FULL_ROTATEX,
      U_FULL_ROTATEY,
      U_FULL_ROTATEZ,
      U_FULL_TRANSX,
      U_FULL_TRANSY,
      U_FULL_TRANSZ,
      U_FULL_PERSPX,
      U_FULL_PERSPY,
      U_FULL_PERSPZ,
      U_FULL_PERSPW
    };

    inline void scale(glm::vec3 &v, pge::f32 newlen)
    {
      pge::f32 len = glm::length(v);
      if (len != 0.0f) {
        v.x *= newlen / len;
        v.y *= newlen / len;
        v.z *= newlen / len;
      }
    }

    inline glm::vec3 combine(glm::vec3 &a, glm::vec3 &b, pge::f32 ascl, pge::f32 bscl)
    {
      return glm::vec3(
        (ascl * a.x) + (bscl * b.x),
        (ascl * a.y) + (bscl * b.y),
        (ascl * a.z) + (bscl * b.z));
    }

    /* unmatrix.c - given a 4x4 matrix, decompose it into standard operations.
    *
    * Author:	Spencer W. Thomas
    * 		University of Michigan
    * https://github.com/erich666/GraphicsGems
    */
    //  unmatrix - Decompose a non-degenerate 4x4 transformation matrix into
    // 	the sequence of transformations that produced it.
    // [Sx][Sy][Sz][Shearx/y][Sx/z][Sz/y][Rx][Ry][Rz][Tx][Ty][Tz][P(x,y,z,w)]
    // 
    // The coefficient of each transformation is returned in the corresponding
    // element of the vector tran.

    inline void unmatrix_full(const glm::mat4 &m, f32 *tran)
    {
      i32 i, j;
      glm::mat4 locmat;
      glm::mat4 pmat, invpmat, tinvpmat;
      // Vector4 type and functions need to be added to the common set.
      glm::vec4 prhs, psol;
      glm::vec3 row[3];

      locmat = m;
      // Normalize the mrix.
      XASSERT(locmat[3][3] != 0, "Matrix not normalized");

      for (i=0; i < 4; i++)
        for (j=0; j < 4; j++)
          locmat[i][j] /= locmat[3][3];

      //  pmat is used to solve for perspective, but it also provides
      // an easy way to test for singularity of the upper 3x3 component.

      pmat = locmat;
      pmat[0][3] = pmat[1][3] = pmat[2][3] = 0.f;
      pmat[3][3] = 1.f;

      ASSERT(glm::determinant(pmat) != 0.0f);

      // First, isolate perspective.  This is the messiest.
      if (locmat[0][3] != 0.f || locmat[1][3] != 0.f || locmat[2][3] != 0.f) {
        // prhs is the right hand side of the equation.
        prhs.x = locmat[0][3];
        prhs.y = locmat[1][3];
        prhs.z = locmat[2][3];
        prhs.w = locmat[3][3];

        //  Solve the equation by inverting pmat and multiplying
        // prhs by the inverse.  (This is the easiest way, not
        // necessarily the best.)
        // inverse function (and det4x4, above) from the mrix
        // Inversion gem in the first volume.

        invpmat  = glm::inverse(pmat);
        tinvpmat = glm::transpose(invpmat);
        psol     = tinvpmat * prhs;

        // Stuff the answer away.
        tran[U_FULL_PERSPX] = psol.x;
        tran[U_FULL_PERSPY] = psol.y;
        tran[U_FULL_PERSPZ] = psol.z;
        tran[U_FULL_PERSPW] = psol.w;
        // Clear the perspective partition.
        locmat[0][3] = locmat[1][3] = locmat[2][3] = 0.f;
        locmat[3][3] = 1.f;
      }
      else		// No perspective.
        tran[U_FULL_PERSPX] = tran[U_FULL_PERSPY] = tran[U_FULL_PERSPZ] = tran[U_FULL_PERSPW] = 0.f;

      // Next take care of translation (easy).
      for (i=0; i < 3; i++) {
        tran[U_FULL_TRANSX + i] = locmat[3][i];
        locmat[3][i] = 0.f;
      }

      // Now get scale and shear.
      for (i=0; i < 3; i++) {
        row[i].x = locmat[i][0];
        row[i].y = locmat[i][1];
        row[i].z = locmat[i][2];
      }

      // Compute X scale factor and normalize first row.
      tran[U_FULL_SCALEX] = glm::length(row[0]);
      matrix_internal::scale(row[0], 1.f);

      // Compute XY shear factor and make 2nd row orthogonal to 1st.
      tran[U_FULL_SHEARXY] = glm::dot(row[0], row[1]);
      row[1] = matrix_internal::combine(row[1], row[0], 1.f, -tran[U_FULL_SHEARXY]);

      // Now, compute Y scale and normalize 2nd row.
      tran[U_FULL_SCALEY] = glm::length(row[1]);
      matrix_internal::scale(row[1], 1.f);
      tran[U_FULL_SHEARXY] /= tran[U_FULL_SCALEY];

      // Compute XZ and YZ shears, orthogonalize 3rd row.
      tran[U_FULL_SHEARXZ] = glm::dot(row[0], row[2]);
      row[1] = matrix_internal::combine(row[2], row[0], 1.f, -tran[U_FULL_SHEARXZ]);
      tran[U_FULL_SHEARYZ] = glm::dot(row[1], row[2]);
      row[1] = matrix_internal::combine(row[2], row[1], 1.f, -tran[U_FULL_SHEARYZ]);

      // Next, get Z scale and normalize 3rd row.
      tran[U_FULL_SCALEZ] =glm::length(row[2]);
      matrix_internal::scale(row[2], 1.f);
      tran[U_FULL_SHEARXZ] /= tran[U_FULL_SCALEZ];
      tran[U_FULL_SHEARYZ] /= tran[U_FULL_SCALEZ];

      //  At this point, the mrix (in rows[]) is orthonormal.
      // Check for a coordinate system flip.  If the determinant
      // is -1, then negate the mrix and the scaling factors.
      if (glm::dot(row[0], glm::cross(row[1], row[2])) < 0.f)
        for (i = 0; i < 3; i++) {
          tran[U_FULL_SCALEX + i] *= -1;
          row[i].x *= -1;
          row[i].y *= -1;
          row[i].z *= -1;
        }

      // Now, get the rotations out, as described in the gem.
      tran[U_FULL_ROTATEY] = asin(-row[0].z);
      if (cos(tran[U_FULL_ROTATEY]) != 0.f) {
        tran[U_FULL_ROTATEX] = atan2(row[1].z, row[2].z);
        tran[U_FULL_ROTATEZ] = atan2(row[0].y, row[0].x);
      }
      else {
        tran[U_FULL_ROTATEX] = atan2(-row[2].x, row[1].y);
        tran[U_FULL_ROTATEZ] = 0.f;
      }
    }

    inline void unmatrix(const glm::mat4 &m, f32 *tran)
    {
      i32 i, j;
      glm::mat4 locmat;
      glm::mat4 pmat, invpmat, tinvpmat;
      // Vector4 type and functions need to be added to the common set.
      glm::vec4 prhs, psol;
      glm::vec3 row[3];
      f32 shear_xy, shear_xz, shear_yz;

      locmat = m;
      // Normalize the mrix.
      XASSERT(locmat[3][3] != 0, "Matrix not normalized");

      for (i=0; i < 4; i++)
        for (j=0; j < 4; j++)
          locmat[i][j] /= locmat[3][3];

      //  pmat is used to solve for perspective, but it also provides
      // an easy way to test for singularity of the upper 3x3 component.

      pmat = locmat;
      pmat[0][3] = pmat[1][3] = pmat[2][3] = 0.f;
      pmat[3][3] = 1.f;

      ASSERT(glm::determinant(pmat) != 0.0f);

      // Next take care of translation (easy).
      for (i=0; i < 3; i++) {
        tran[U_TRANSX + i] = locmat[3][i];
        locmat[3][i] = 0.f;
      }

      // Now get scale and shear.
      for (i=0; i < 3; i++) {
        row[i].x = locmat[i][0];
        row[i].y = locmat[i][1];
        row[i].z = locmat[i][2];
      }

      // Compute X scale factor and normalize first row.
      tran[U_SCALEX] = glm::length(row[0]);
      matrix_internal::scale(row[0], 1.f);

      // Compute XY shear factor and make 2nd row orthogonal to 1st.
      shear_xy = glm::dot(row[0], row[1]);
      row[1] = matrix_internal::combine(row[1], row[0], 1.f, -shear_xy);

      // Now, compute Y scale and normalize 2nd row.
      tran[U_SCALEY] = glm::length(row[1]);
      matrix_internal::scale(row[1], 1.f);

      // Compute XZ and YZ shears, orthogonalize 3rd row.
      shear_xz = glm::dot(row[0], row[2]);
      row[1] = matrix_internal::combine(row[2], row[0], 1.f, -shear_xz);
      shear_yz = glm::dot(row[1], row[2]);
      row[1] = matrix_internal::combine(row[2], row[1], 1.f, -shear_yz);

      // Next, get Z scale and normalize 3rd row.
      tran[U_SCALEZ] = glm::length(row[2]);
      matrix_internal::scale(row[2], 1.f);

      //  At this point, the mrix (in rows[]) is orthonormal.
      // Check for a coordinate system flip.  If the determinant
      // is -1, then negate the mrix and the scaling factors.
      if (glm::dot(row[0], glm::cross(row[1], row[2])) < 0.f)
        for (i = 0; i < 3; i++) {
          tran[U_SCALEX + i] *= -1;
          row[i].x *= -1;
          row[i].y *= -1;
          row[i].z *= -1;
        }

      // Now, get the rotations out, as described in the gem.
      tran[U_ROTATEY] = asin(-row[0].z);
      if (cos(tran[U_ROTATEY]) != 0.f) {
        tran[U_ROTATEX] = atan2(row[1].z, row[2].z);
        tran[U_ROTATEZ] = atan2(row[0].y, row[0].x);
      }
      else {
        tran[U_ROTATEX] = atan2(-row[2].x, row[1].y);
        tran[U_ROTATEZ] = 0.f;
      }
    }

  }

  inline void get_mat4_translation(const glm::mat4 &m, glm::vec3 &result)
  {
    i32 i, j;
    glm::mat4 locmat, pmat;

    locmat = m;

    // Normalize.

    XASSERT(locmat[3][3] != 0, "Matrix not normalized");

    for (i=0; i < 4; i++)
      for (j=0; j < 4; j++)
        locmat[i][j] /= locmat[3][3];

    //  pmat is used to solve for perspective, but it also provides
    // an easy way to test for singularity of the upper 3x3 component.

    pmat = locmat;
    pmat[0][3] = pmat[1][3] = pmat[2][3] = 0.f;
    pmat[3][3] = 1.f;

    XASSERT(glm::determinant(pmat) != 0.0f, "Matrix determinant == 0.0f");

    result.x = locmat[3][0];
    result.y = locmat[3][1];
    result.z = locmat[3][2];

  }

  inline void compose_mat4(glm::mat4 &m, const DecomposedMatrix &dm)
  {
    m = IDENTITY_MAT4;
    m = m * glm::translate(IDENTITY_MAT4, dm.translation);
    m = m * glm::toMat4(dm.rotation);
    m = m * glm::scale(IDENTITY_MAT4, dm.scale);
  }

  inline void decompose_mat4(const glm::mat4 &m, DecomposedMatrix &dm)
  {
    using namespace matrix_internal;

    f32 transformations[9];
    unmatrix(m, transformations);

    const u32 size = sizeof(f32)* 3;

    memcpy(&dm.translation, &transformations[U_TRANSX], size);
    memcpy(&dm.scale, &transformations[U_SCALEX], size);
    dm.rotation = glm::quat(glm::vec3(transformations[U_ROTATEX], transformations[U_ROTATEY], transformations[U_ROTATEZ]));
  }
}