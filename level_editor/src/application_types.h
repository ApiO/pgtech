#pragma once

#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <mintomic/mintomic.h>
#include <engine/pge.h>
#include "bases/focusable.h"


using namespace pge;

#define BOOL(_mint32)(mint_load_32_relaxed(&(_mint32)) == 1u)
#define SET_MINT(_mint32, v)(mint_store_32_relaxed(&(_mint32), (v)))

// consts
namespace app
{
  const f32 CAMERA_Z_MIN           = 100.f;
  const f32 CAMERA_Z_MAX           = 4096;
  const f32 DEFAULT_CAMERA_Z       = 1000.f;
  const f32 FRAME_DURATION         = 1.f / 60.f;
  const f32 SCROLL_SPEED           = 6.f;
  const i32 WINDOW_MARGIN          = 10;
  const i32 IMGUI_ITEM_HEIGHT      = 24;
  const i32 CROSS_COLOR            = imguiRGBA(255, 255, 255, 60);
  const i32 WHITE_TEXT             = imguiRGBA(255, 255, 255, 255);
  const i32 GRAY_TEXT              = imguiRGBA(122, 122, 122, 255);
  const i32 BLACK_TEXT             = imguiRGBA(0, 0, 0, 255);
  const i32 SELECT_COLOR           = imguiRGBA(14, 99, 156, 255);
  const i32 DISABLE_GRAY_COLOR     = imguiRGBA(172, 172, 172, 255);
  const i32 WINDOW_MAX_WIDTH       = 300;
  const i32 WINDOW_MIN_HEIGHT      = 300;
  const i32 INPUT_HEIGHT           = 22;
  const i32 PROPERTY_WINDOW_HEIGHT = 380;
  const i32 IMGUI_PAD_WIDTH        = 16;

  const u32 MAX_U32 = (u32)-1;
  const u64 MAX_U64 = (u64)-1;

  const Color RED_COLOR(255, 0, 0, 255);
  const Color BLUE_COLOR(0, 0, 255, 255);
  const Color GREEN_COLOR(0, 255, 0, 255);
  const Color SELECTION_COLOR(130, 130, 130, 255);
}

// common types
namespace app
{
typedef mint_atomic32_t mint32;

  struct Screen
  {
    i32 width, height;
    i32 max_window_height;
  };

  enum ResourceType {
    RESOURCE_TYPE_NONE = 0,
    RESOURCE_TYPE_UNIT,
    RESOURCE_TYPE_SPRITE,
    RESOURCE_TYPE_ACTOR,
    RESOURCE_TYPE_TEXT,
    RESOURCE_TYPE_LAYER
  };

  struct EditorResource {
    ResourceType type;
    u64 id;
  };
  
  struct ApplicationData
  {
    u64 world;
    u64 camera;
    u64 viewport;
    i32 mouse_scroll;
    glm::vec2 mouse_position;
    glm::mat4 screen_projection;
  };
}

// singletons
namespace app
{
  extern ApplicationData app_data;
  extern Screen screen;
}


// utils /!\ à virer
namespace app
{
  inline bool intersect_plane(const glm::vec3 &n, const glm::vec3 &p0, const glm::vec3& l0, const glm::vec3 &l, float &d)
  {
    glm::vec3 p0l0;
    // assuming vectors are all normalized
    float denom = glm::dot(n, l);
    if (denom > 1e-6) {
      p0l0 = p0 - l0;
      d = glm::dot(p0l0, n) / denom;
      return (d >= 0);
    }
    return false;
  }

  inline bool has_input_focused()
  {
    return focusable::current_focus != NULL;
  }
}