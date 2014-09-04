#include <runtime/assert.h>
#include <runtime/idlut.h>
#include <engine/matrix.h>

#include <forms/forms.h>

#include "selection_handler.h"
#include "level_handler.h"
#include "resource_handler.h"

using namespace pge;

namespace app
{
  using namespace handlers;

  static void set_properties(Array<SelectionState> &list)
  {
    if (array::size(list) == 1)
      property->load(*array::begin(list));
    else
      property->unload();
  }

  static void select_closer(const Resources &resources, const ResourceType type, const glm::vec3 &from, const glm::vec3 &dir,
    EditorResource &result, f32 &distance)
  {
    const IdLookupTable<ResourceInfo>::Entry *e, *end = idlut::end(resources);
    f32 dist;
    Box b;

    for (e = idlut::begin(resources); e < end; e++) {

      if (e->value.instance == MAX_U64) continue;

      resource->get_box(e->value, b);

      dist = math::ray_box_intersection(from, dir, b);

      if (dist >= 0.f && dist < distance) {
        result.id   = e->id;
        result.type = type;
        distance = dist;
      }
    }
  }

  static void get_selection(const u64 &world, const u64 &camera, const glm::vec2 &mouse, EditorResource &result)
  {
    f32 distance = std::numeric_limits<float>::max();
    glm::vec3 from, dir;

    result.type = RESOURCE_TYPE_NONE;

    camera::screen_to_world(world, camera, glm::vec3(mouse.x, mouse.y, 1), dir);
    camera::screen_to_world(world, camera, glm::vec3(mouse.x, mouse.y, 0), from);
    dir = glm::normalize(dir - from);

    select_closer(resource->get_units(), RESOURCE_TYPE_UNIT, from, dir, result, distance);
    select_closer(resource->get_sprites(), RESOURCE_TYPE_SPRITE, from, dir, result, distance);
    
    //select_closer(level->actors, RESOURCE_TYPE_ACTOR, from, dir, result, distance);
    //select_closer(level->texts, RESOURCE_TYPE_TEXT, from, dir, result, distance);
  }
  
  namespace handlers
  {
    void SelectionHandler::select_item(void)
    {
      EditorResource result;

      get_selection(app_data.world, app_data.camera, mouse::get_position(), result);

      // if no selection
      if (result.type == RESOURCE_TYPE_NONE) return;

      // handles multi selection mode
      if (keyboard::button(KEYBOARD_KEY_LEFT_CONTROL)){
        if (array::size(items)){
          SelectionState *item, *end = array::end(items);
          i32 i = 0;
          for (item = array::begin(items); item < end; item++){
            if (item->id == result.id && item->type == result.type) {
              items[i] = array::pop_back(items);

              set_properties(items);
              dirty = true;

              return;
            }
            i++;
          }
        }
      }
      else{
        array::clear(items);
      }

      // add new selected item to list
      SelectionState item;
      item.id   = result.id;
      item.type = result.type;
      
      array::push_back(items, item);

      set_properties(items);

      dirty = true;
    }

    void SelectionHandler::select_all(void)
    {
      printf("TODO: select all\n");
    }

    void SelectionHandler::clear(void)
    {
      array::clear(items);
      array::clear(states);
      dirty = true;
    }

    void SelectionHandler::remove(void)
    {
      SelectionState *item, *end = array::end(items);
      for (item = array::begin(items); item < end; item++){
        resource->destroy_resource(*(EditorResource*)item);
      }
      array::clear(items);
      dirty = true;
      property->unload();
    }

    void SelectionHandler::move()
    {
      if (!keyboard::button(KEYBOARD_KEY_LEFT_SHIFT) || !array::size(items)) return;

      glm::vec2 mouse_position(mouse::get_position());

      if (!start_translation && mouse::pressed(MOUSE_KEY_1)) {
        start_rotation = false;
        start_translation = true;
        state_position = mouse_position;

        // initializes all origin point according to items' z, used to find delta position in the next frame
        glm::vec3 near_point, far_point, direction, item_pos;
        f32 distance;

        camera::screen_to_world(app_data.world, app_data.camera, glm::vec3(state_position.x, state_position.y, 0), near_point);
        camera::screen_to_world(app_data.world, app_data.camera, glm::vec3(state_position.x, state_position.y, 1), far_point);
        direction = glm::normalize(far_point - near_point);

        SelectionState *item, *end = array::end(items);
        for (item = array::begin(items); item < end; item++)
        {
          resource->get_position(*item, item_pos);

          glm::vec3 plane_normal(0, 0, -1);
          glm::vec3 plane_position(0, 0, item_pos.z);

          bool r = intersect_plane(plane_normal, plane_position, near_point, direction, distance);
          ASSERT(r);

          item->state_position = glm::vec3(near_point + direction * distance);
        }
        return;
      }
      else if (start_translation && mouse::released(MOUSE_KEY_1)) {
        start_translation = false;
        return;
      }

      if (!start_translation) return;

      level->set_edited();

      if (state_position == mouse_position) return;

      dirty = true;
      glm::vec3 near_point, far_point, direction, item_pos;
      f32 distance;

      camera::screen_to_world(app_data.world, app_data.camera, glm::vec3(mouse_position.x, mouse_position.y, 0), near_point);
      camera::screen_to_world(app_data.world, app_data.camera, glm::vec3(mouse_position.x, mouse_position.y, 1), far_point);
      direction = glm::normalize(far_point - near_point);

      SelectionState *item, *end = array::end(items);
      for (item = array::begin(items); item < end; item++)
      {
        resource->get_position(*item, item_pos);

        glm::vec3 plane_normal(0, 0, -1);
        glm::vec3 plane_position(0, 0, item_pos.z);

        bool r = intersect_plane(plane_normal, plane_position, near_point, direction, distance);
        ASSERT(r);

        // updates resource position
        glm::vec3 new_pos(near_point + direction * distance);
        item_pos += new_pos - item->state_position;

        item->state_position = new_pos;

        resource->set_position(*item, item_pos);
      }

      property->refresh();
      state_position = mouse_position;
    }

    void SelectionHandler::rotate()
    {
      if (!keyboard::button(KEYBOARD_KEY_LEFT_SHIFT) || !array::size(items)) return;

      glm::vec2 mouse_position(mouse::get_position());

      if (!start_rotation && mouse::pressed(MOUSE_KEY_2)) {
        start_translation = false;
        start_rotation = true;
        state_position = mouse_position;
        return;
      }
      else if (start_rotation && mouse::released(MOUSE_KEY_2)) {
        start_rotation = false;
        return;
      }

      if (!start_rotation) return;

      level->set_edited();

      if (state_position == mouse_position) return;

      dirty = true;
      glm::vec2 v1(glm::normalize(state_position));
      glm::vec2 v2(glm::normalize(mouse_position));
      f32 rotation = glm::degrees((atan2(v2.y, v2.x) - atan2(v1.y, v1.x)));

      // get current angle value
      f32  roll;
      bool flip;
      SelectionState *item, *end = array::end(items);

      for (item = array::begin(items); item < end; item++){
        resource->get_rotation(*item, roll, flip);
        roll += rotation;
        resource->set_rotation(*item, roll, flip);
      }

      state_position = mouse_position;

      property->refresh();
    }

    void SelectionHandler::scale(void)
    {
      if (!keyboard::button(KEYBOARD_KEY_LEFT_SHIFT) || !array::size(items)) return;

      if (app_data.mouse_scroll == 0) return;

      dirty = true;
      start_translation = false;
      start_rotation = false;

      level->set_edited();

      // get current angle value
      const f32 SCALE_PAD = .1f;
      const f32 SCALE_MAX = 30.f;
      glm::vec3 scale;
      SelectionState *item, *end = array::end(items);

      for (item = array::begin(items); item < end; item++){
        resource->get_scale(*item, scale);

        scale.x -= SCALE_PAD * app_data.mouse_scroll;
        scale.y -= SCALE_PAD * app_data.mouse_scroll;

        if (scale.x < SCALE_PAD)
          scale.y = scale.x = SCALE_PAD;

        if (scale.x > SCALE_MAX)
          scale.y = scale.x = SCALE_MAX;

        resource->set_scale(*item, scale);
      }
      
      property->refresh();
    }

    void SelectionHandler::reset_rotation_and_scale(void)
    {
      if (!array::size(items)) return;

      dirty = true;
      start_translation = false;
      start_rotation = false;

      level->set_edited();

      // get current angle value
      const glm::vec3 scale(1.f);
      const glm::quat q(1, 0, 0, 0);

      f32 roll;
      bool flip;
      SelectionState *item, *end = array::end(items);
      for (item = array::begin(items); item < end; item++)
      {
        resource->set_scale(*item, scale);
        resource->get_rotation(*item, roll, flip);
        resource->set_rotation(*item, 0, flip);
      }
      if (array::size(items) == 1) property->refresh();
    }

    void SelectionHandler::update(f64 dt)
    {
      (void)dt;

      // Select all
      if (keyboard::button(KEYBOARD_KEY_LEFT_CONTROL) && keyboard::pressed(KEYBOARD_KEY_A))
        select_all();

      // Handles selection move
      move();

      // Handles selection rotation
      rotate();

      // Handles selection scale
      scale();

      // Handles selection remove
      if (!has_input_focused() && keyboard::button(KEYBOARD_KEY_DELETE) && array::size(items))
        remove();

      if (!has_input_focused() && keyboard::pressed(KEYBOARD_KEY_SPACE))
        reset_rotation_and_scale();
    }

    void SelectionHandler::synchronize(void)
    {
      if (!dirty) return;
      dirty = false;

      // regenerates selection data
      if (array::size(items)){
        // updates items' box and pose
        SelectionState *item, *end = array::end(items);
        for (item = array::begin(items); item < end; item++){
          resource->get_box(*item, item->box);
          resource->get_pose(*item, item->pose);
        }
      }

      array::resize(states, array::size(items));
      memcpy(array::begin(states), array::begin(items), sizeof(SelectionState)*array::size(items));
    }

    void SelectionHandler::draw(void)
    {
      if (!array::size(states)) return;

      glm::vec3 vertices[4];
      DecomposedMatrix dm;
      glm::mat4 pose;
      Box *box = NULL;

      SelectionState *item, *end = array::end(states);
      for (item = array::begin(states); item < end; item++)
      {
        // Draws box
        box = &item->box;
        for (i32 i = 0; i < 4; i++)
          vertices[i].z = (box->min.z + box->max.z) / 2;

        vertices[0].x = box->min.x;
        vertices[0].y = box->min.y;

        vertices[1].x = box->max.x;
        vertices[1].y = box->min.y;

        vertices[2].x = box->max.x;
        vertices[2].y = box->max.y;

        vertices[3].x = box->min.x;
        vertices[3].y = box->max.y;

        application::render_polygon(vertices, 4, SELECTION_COLOR, app_data.world, app_data.camera, app_data.viewport);

        // draws referencial
        decompose_mat4(item->pose, dm);
        dm.scale = glm::vec3(1.f);
        compose_mat4(pose, dm);

        glm::vec3 o = glm::vec3(pose * glm::vec4(0, 0, 0, 1));
        glm::vec3 a = glm::vec3(pose * glm::vec4(40, 0, 0, 1));
        glm::vec3 b = glm::vec3(pose * glm::vec4(0, 40, 0, 1));

        application::render_line(o, a, RED_COLOR, app_data.world, app_data.camera, app_data.viewport);
        application::render_line(o, b, GREEN_COLOR, app_data.world, app_data.camera, app_data.viewport);
      }
    }
  }
}

namespace app
{

  namespace handlers
  {
    SelectionHandler *selection;

    namespace selection_handler
    {
      void init(Allocator &a)
      {
        selection = MAKE_NEW(a, SelectionHandler, a);
      }

      void shutdown(Allocator &a)
      {
        if (!selection) return;
        MAKE_DELETE(a, SelectionHandler, selection);
        selection = NULL;
      }
    }

  }

}