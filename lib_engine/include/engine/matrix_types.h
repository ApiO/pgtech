#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "pge_types.h"

namespace pge
{
  struct DecomposedMatrix
  {
    DecomposedMatrix() 
      : translation(IDENTITY_TRANSLATION), rotation(IDENTITY_ROTATION), scale(IDENTITY_SCALE){}
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
  };
}