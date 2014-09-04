#include "runtime/trace.h"
#include "runtime/array.h"
#include "runtime/list.h"

#include "data/texture.h"

#include "types.h"
#include "texture_packer.h"

#include <GL/SOIL.h>

namespace
{
  using namespace pge;

  struct Region
  {
    Region() :
      x(0), y(0), width(0), height(0) {}
    Region(i32 _x, i32 _y, i32 _width, i32 _height) :
      x(_x), y(_y), width(_width), height(_height) {}
    i32 x, y;
    i32 width, height;
  };

  struct PageNode
  {
    PageNode(){};
    PageNode(Region _region, u32 _index, bool _rotate) :
      region(_region), index(_index), rotate(_rotate){};
    Region region;
    u32    index;
    bool   rotate;
    void   set(Region _region, u32 _index, bool _rotate) { region=_region; index=_index; rotate=_rotate; }
  };

  struct Page
  {
    Array<PageNode> *nodes;
    List<Region>    *free_nodes;
    i32              width;
    i32              height;
    i32              isfull;
  };
  // Packer heuristic list
  enum Heuristic
  {
    HEURISTIC_TBL,  // Taller Bottom left rule: Taller Tetris placement, the highest area is placed as low as possible, always at the botom left.
    HEURISTIC_BSSF, // Best short side fit:     Positions the rectangle against the short side of a free rectangle into which it fits the best.
    HEURISTIC_CP,   // Contact point rule:      Choosest the placement where the rectangle touches other rects as much as possible.
    HEURISTIC_BLSF, // Best long side fit:      Positions the rectangle against the long side of a free rectangle into which it fits the best.
    HEURISTIC_BAF,  // Best area fit:           Positions the rectangle into the smallest free rect into which it fits.
    HEURISTIC_COUNT, //= 1  // not a heuristic
  };

  struct RegionScore
  {
    RegionScore(i32 _a, i32 _b, i32 _free_node) :
      a(_a), b(_b), free_node(_free_node) {}
    i32   a, b;
    i32   free_node;
    void  set(i32 _a, i32 _b, i32 _free_node) { a=_a; b=_b; free_node=_free_node; }
  };

  struct HeuristicScore
  {
    HeuristicScore(u32 _h_i, u32 _area, u32 _num_pages, u32 _num_rotation) :
      heuristic_index(_h_i), area(_area), num_pages(_num_pages), num_rotation(_num_rotation) {}
    u32 heuristic_index;
    u32 num_pages;
    u32 num_rotation;
    u32 area;
  };

  typedef Array<Page> Pages;
  typedef Array<u32>  Indices;


  inline void page_init(Page &page, Allocator &a)
  {
    page.free_nodes = MAKE_NEW(a, List<Region>, a);
    page.nodes      = MAKE_NEW(a, Array<PageNode>, a);
  }

  inline void result_destroy(Array<Page> &pages, Allocator &a)
  {
    Page  *page  = array::begin(pages),
      *pend  = array::end(pages);
    for (; page < pend; page++) {
      MAKE_DELETE(a, List<Region>, page->free_nodes);
      MAKE_DELETE(a, Array<PageNode>, page->nodes);
    }
  }


  //-----------------------
  //         Misc
  //-----------------------

  inline bool region_fit_node(const i32 width, const i32 height, const Region &region)
  {
    return region.width >= width && region.height >= height;
  }

  /// Returns 0 if the two intervals i1 and i2 are disjoint, or the length of their overlap otherwise.
  inline i32 common_interval_length(const i32 i1_start, const i32 i1_end, const i32 i2_start, const i32 i2_end)
  {
    return  (i1_end < i2_start || i2_end < i1_start)
      ? 0 : Min(i1_end, i2_end) - Max(i1_start, i2_start);
  }

  inline i32 contact_point_score(const Page &page, const i32 x, const i32 y, const i32 width, const i32 height)
  {
    i32 score = 0;

    if (x == 0 || x + width == page.width)  score = height;
    if (y == 0 || y + height == page.height)score = width;

    for (u32 i = 0; i < array::size(*page.nodes); i++) {
      const Region &rect = (*page.nodes)[i].region;

      if (rect.x == x + width || rect.x + rect.width == x)
        score += common_interval_length(rect.y, rect.y + rect.height, y, y + height);
      if (rect.y == y + height || rect.y + rect.height == y)
        score += common_interval_length(rect.x, rect.x + rect.width, x, x + width);
    }
    return score;
  }

  inline void compare_result(HeuristicScore &best_score, u32 h_i, Pages &pages)
  {
    u32 area = 0, num_rotation = 0;
    for (u32 i = 0; i < array::size(pages); i++) {
      area += pages[i].height * pages[i].width;
      for (u32 j = 0; j < array::size(*pages[i].nodes); j++)
        if ((*pages[i].nodes)[j].rotate) num_rotation++;
    }

    bool new_best_score = false;

    if (array::size(pages) == best_score.num_pages) {
      if (num_rotation == best_score.num_rotation) {
        if (area < best_score.area) { new_best_score = true; }
      }
      else if (num_rotation < best_score.num_rotation) {
        new_best_score = true;
      }
    }
    else if (array::size(pages) < best_score.num_pages) {
      new_best_score = true;
    }

    if (new_best_score) {
      best_score.heuristic_index  = h_i;
      best_score.area             = area;
      best_score.num_pages        = array::size(pages);
      best_score.num_rotation     = num_rotation;
    }
  }

  bool split_free_node(Heuristic h, List<Region> &free_nodes, u32 free_node_index, Region &region)
  {
    const Region &free_node = free_nodes[free_node_index];

    if (free_node.height < region.height || free_node.width < region.width) {
      LOG("Texture packer aberation on heuristic %d. Region does not fit selected free node. free_node[%u, %u] <-> region[%u, %u]",
          (i32)h, free_node.width, free_node.height, region.width, region.height);
      return false;
    }

    /*  cut rule
    -------------------------------------
    |                                   |
    -------------------------------------
    |       |///////////|               |
    |       |///////////|               |
    |       |///////////|               |
    |       |///////////|               |
    -------------------------------------
    |                                   |
    |                                   |
    |                                   |
    |                                   |
    -------------------------------------
    */

    //top free node
    if (region.y > free_node.y) {
      Region node(free_node.x, free_node.y, free_node.width, region.y - free_node.y);
      list::push_back(free_nodes, node);
    }

    //bottom free node
    if ((region.y + region.height) < (free_node.y + free_node.height)) {
      i32 y = region.y + region.height;
      Region node(free_node.x, y, free_node.width, (free_node.y + free_node.height) - y);;
      list::push_back(free_nodes, node);
    }

    // left free node
    if (region.x > free_node.x) {
      Region node(free_node.x, region.y, region.x - free_node.x, region.height);
      list::push_back(free_nodes, node);
    }

    // right free node
    if ((region.x + region.width) < (free_node.x + free_node.width)) {
      i32 x = region.x + region.width;
      Region node(x, region.y, (free_node.x + free_node.width) - x, region.height);
      list::push_back(free_nodes, node);
    }

    list::remove(free_nodes, free_node_index);

    return true;
  }

  //-----------------------
  //      heuristics
  //-----------------------

  inline bool set_with_bssf(Page &page, PageNode &best_node, RegionScore &best_score, bool &is_set, i32 width, i32 height, bool rotation)
  {
    const List<Region>::Entry *free_node = list::begin(*page.free_nodes),
      *end       = list::end(*page.free_nodes);

    PageNode    node(Region(), INT_MAX, false);
    RegionScore score(INT_MAX, INT_MAX, -1);

    i32 data_index = -1;
    i32 left_over_horiz, left_over_vert,
      short_side_fit, long_side_fit;

    if (best_score.free_node == -2)
      best_score.set(INT_MAX, INT_MAX, -1);

    for (u32 i = 0; free_node < end; i++) {
      if (region_fit_node(width, height, free_node->value)) {
        left_over_horiz = abs((i32)(free_node->value.width - width));
        left_over_vert  = abs((i32)(free_node->value.height - height));
        short_side_fit  = Min<i32>(left_over_horiz, left_over_vert);
        long_side_fit   = Max<i32>(left_over_horiz, left_over_vert);

        if (short_side_fit < score.a || (short_side_fit == score.a && long_side_fit < score.b)) {
          node.set(Region(free_node->value.x, free_node->value.y, width, height), INT_MAX, false);
          score.set(short_side_fit, long_side_fit, -1);
          data_index = i;
        }
      }
      if (rotation && height != width&& region_fit_node(height, width, free_node->value)) {
        left_over_horiz = abs((i32)(free_node->value.width - height));
        left_over_vert  = abs((i32)(free_node->value.height - width));
        short_side_fit  = Min<i32>(left_over_horiz, left_over_vert);
        long_side_fit   = Max<i32>(left_over_horiz, left_over_vert);

        if (short_side_fit < score.a || (short_side_fit == score.a && long_side_fit < score.b)) {
          node.set(Region(free_node->value.x, free_node->value.y, height, width), INT_MAX, true);
          score.set(short_side_fit, long_side_fit, -1);
          data_index = i;
        }
      }
      free_node++;
    }
    if (data_index != -1 && (score.a < best_score.a || (score.a == best_score.a && score.b < best_score.b))) {
      score.free_node = list::get_index(*page.free_nodes, data_index);
      if (score.free_node == -1) {
        LOG("Data index not in list value=%d", score.free_node);
        return false;
      }

      best_score = score;
      best_node  = node;
      is_set = true;
    }
    return true;
  }

  inline bool set_with_tbl(Page &page, PageNode &best_node, RegionScore &best_score, bool &is_set, i32 width, i32 height, bool rotation)
  {
    const List<Region>::Entry *free_node = list::begin(*page.free_nodes),
      *end       = list::end(*page.free_nodes);

    PageNode    node(Region(), INT_MAX, false);
    RegionScore score(0, 0, -1);

    i32 data_index = -1;

    if (best_score.free_node == -2)
      best_score.set(0, 0, -1);

    for (u32 i = 0; free_node < end; i++) {
      if (region_fit_node(width, height, free_node->value)
          && (height > score.a || (height == score.a && free_node->value.y > score.b))) {
        node.set(Region(free_node->value.x, (free_node->value.y + free_node->value.height) - height, width, height), INT_MAX, false);
        score.set(height, free_node->value.y, -1);
        data_index = i;
      }
      if (rotation && height != width && region_fit_node(height, width, free_node->value)
          && (width > score.a || (width == score.a && free_node->value.y > score.b))) {
        data_index = i;
        node.set(Region(free_node->value.x, (free_node->value.y + free_node->value.height) - width, height, width), INT_MAX, true);
        score.set(width, free_node->value.y, -1);
      }
      free_node++;
    }
    if (data_index != -1 && (score.a > best_score.a || (score.a == best_score.a && score.b > best_score.b))) {
      score.free_node = list::get_index(*page.free_nodes, data_index);
      if (score.free_node == -1) {
        LOG("Data index not in list value=%d", score.free_node);
        return false;
      }
      best_score = score;
      best_node  = node;
      is_set = true;
    }
    return true;
  }

  inline bool set_with_cp(Page &page, PageNode &best_node, RegionScore &best_score, bool &is_set, i32 width, i32 height, bool rotation)
  {
    const List<Region>::Entry *free_node = list::begin(*page.free_nodes),
      *end       = list::end(*page.free_nodes);

    PageNode    node(Region(), INT_MAX, false);
    RegionScore score(-1, INT_MAX, -1);

    i32 data_index = -1;

    i32 contact_score;

    if (best_score.free_node == -2)
      best_score.set(-1, INT_MAX, -1);

    for (u32 i = 0; free_node < end; i++) {
      if (region_fit_node(width, height, free_node->value)) {
        contact_score = contact_point_score(page, free_node->value.x, free_node->value.y, width, height);
        if (contact_score > score.a) {
          node.set(Region(free_node->value.x, free_node->value.y, width, height), INT_MAX, false);
          score.set(contact_score, 0, -1);
          data_index = i;
        }
      }

      if (rotation && height != width && region_fit_node(height, width, free_node->value)) {
        contact_score = contact_point_score(page, free_node->value.x, free_node->value.y, height, width);
        if (contact_score > score.a) {
          node.set(Region(free_node->value.x, free_node->value.y, height, width), INT_MAX, true);
          score.set(contact_score, 0, -1);
          data_index = i;
        }
      }
      free_node++;
    }
    score.a *= -1; // Reverse since we are minimizing, but for contact point &score_ bigger is better.
    if (data_index != -1 && (score.a < best_score.a || (score.a == best_score.a && score.b < best_score.b))) {
      score.free_node = list::get_index(*page.free_nodes, data_index);
      if (score.free_node == -1) {
        LOG("Data index not in list value=%d", score.free_node);
        return false;
      }
      best_score = score;
      best_node  = node;
      is_set = true;
    }
    return true;
  }

  inline bool set_with_blsf(Page &page, PageNode &best_node, RegionScore &best_score, bool &is_set, i32 width, i32 height, bool rotation)
  {
    PageNode    node(Region(), INT_MAX, false);
    RegionScore score(INT_MAX, INT_MAX, -1);

    i32 data_index = -1;

    i32 left_over_horiz, left_over_vert,
      short_side_fit, long_side_fit;

    if (best_score.free_node == -2)
      best_score.set(INT_MAX, INT_MAX, -1);

    const List<Region>::Entry *free_node = list::begin(*page.free_nodes),
      *end       = list::end(*page.free_nodes);
    for (u32 i = 0; free_node < end; i++) {
      if (region_fit_node(width, height, free_node->value)) {
        left_over_horiz = abs((i32)(free_node->value.width - width));
        left_over_vert  = abs((i32)(free_node->value.height - height));
        short_side_fit  = Min<i32>(left_over_horiz, left_over_vert);
        long_side_fit   = Max<i32>(left_over_horiz, left_over_vert);
        if (long_side_fit < score.a || (long_side_fit == score.a && short_side_fit < score.b)) {
          node.set(Region(free_node->value.x, free_node->value.y, width, height), INT_MAX, false);
          score.set(long_side_fit, short_side_fit, -1);
          data_index = i;
        }
      }
      if (rotation && height != width && region_fit_node(height, width, free_node->value)) {
        left_over_horiz = abs((i32)(free_node->value.width - height));
        left_over_vert  = abs((i32)(free_node->value.height - width));
        short_side_fit  = Min<i32>(left_over_horiz, left_over_vert);
        long_side_fit   = Max<i32>(left_over_horiz, left_over_vert);
        if (long_side_fit < score.a || (long_side_fit == score.a && short_side_fit < score.b)) {
          node.set(Region(free_node->value.x, free_node->value.y, height, width), INT_MAX, true);
          score.set(long_side_fit, short_side_fit, -1);
          data_index = i;
        }
      }
      free_node++;
    }

    if (data_index != -1 && (score.a < best_score.a || (score.a == best_score.a && score.b < best_score.b))) {
      score.free_node = list::get_index(*page.free_nodes, data_index);
      if (score.free_node == -1) {
        LOG("Data index not in list value=%d", score.free_node);
        return false;
      }
      best_score = score;
      best_node  = node;
      is_set = true;
    }
    return true;
  }

  inline bool set_with_baf(Page &page, PageNode &best_node, RegionScore &best_score, bool &is_set, i32 width, i32 height, bool rotation)
  {
    const List<Region>::Entry *free_node = list::begin(*page.free_nodes),
      *end       = list::end(*page.free_nodes);

    PageNode    node(Region(), INT_MAX, false);
    RegionScore score(INT_MAX, INT_MAX, -1);

    i32 data_index = -1;

    i32 left_over_horiz,
      left_over_vert,
      short_side_fit;

    if (best_score.free_node == -2)
      best_score.set(INT_MAX, INT_MAX, -1);

    for (u32 i = 0; free_node < end; i++) {
      i32 areaFit = free_node->value.width * (int)free_node->value.height - width * height;
      if (region_fit_node(width, height, free_node->value)) {
        left_over_horiz = abs((i32)(free_node->value.width - width));
        left_over_vert  = abs((i32)(free_node->value.height - height));
        short_side_fit  = Min<i32>(left_over_horiz, left_over_vert);
        if (areaFit < score.a || (areaFit == score.a && short_side_fit < score.b)) {
          node.set(Region(free_node->value.x, free_node->value.y, width, height), INT_MAX, false);
          score.set(areaFit, short_side_fit, -1);
          data_index = i;
        }
      }
      if (rotation && height != width  && region_fit_node(height, width, free_node->value)) {
        left_over_horiz = abs((i32)(free_node->value.width - height));
        left_over_vert  = abs((i32)(free_node->value.height - width));
        short_side_fit  = Min<i32>(left_over_horiz, left_over_vert);
        if (areaFit < score.a || (areaFit == score.a && short_side_fit < score.b)) {
          node.set(Region(free_node->value.x, free_node->value.y, height, width), INT_MAX, true);
          score.set(areaFit, short_side_fit, -1);
          data_index = i;
        }
      }
      free_node++;
    }

    if (data_index != -1 && (score.a < best_score.a || (score.a == best_score.a && score.b < best_score.b))) {
      score.free_node = list::get_index(*page.free_nodes, data_index);
      if (score.free_node == -1) {
        LOG("Data index not in list value=%d", score.free_node);
        return false;
      }
      best_score = score;
      best_node  = node;
      is_set = true;
    }
    return true;
  }

  //-----------------------
  //       Inserts
  //-----------------------

  static bool insert_region(Page &page, i32 &best_index, const Array<InputRectangle> &rectangles, const Indices &indices, Heuristic h, bool rotation, u32 spacing)
  {
    i32 best_region_index = -1;
    PageNode    best_node;
    RegionScore best_score(0, 0, -2);

    best_index = -1;

    for (u32 i = 0; i < array::size(indices); ++i) {
      const i32 index = indices[i];
      const InputRectangle &rect = rectangles[index];
      bool is_set = false;

      switch (h) {
        case HEURISTIC_TBL:  if (!set_with_tbl (page, best_node, best_score, is_set, rect.width + spacing, rect.height + spacing, rotation)) return false; break;
        case HEURISTIC_BSSF: if (!set_with_bssf(page, best_node, best_score, is_set, rect.width + spacing, rect.height + spacing, rotation)) return false; break;
        case HEURISTIC_CP:   if (!set_with_cp  (page, best_node, best_score, is_set, rect.width + spacing, rect.height + spacing, rotation)) return false; break;
        case HEURISTIC_BLSF: if (!set_with_blsf(page, best_node, best_score, is_set, rect.width + spacing, rect.height + spacing, rotation)) return false; break;
        case HEURISTIC_BAF:  if (!set_with_baf (page, best_node, best_score, is_set, rect.width + spacing, rect.height + spacing, rotation)) return false; break;
      }

      if (is_set) {
        best_index = i;
        best_region_index = index;
      }
    }

    if (best_region_index != -1) {
      //register new node
      best_node.index  = best_region_index;
      array::push_back(*page.nodes, best_node);

      //update free nodes
      return split_free_node(h, *page.free_nodes, best_score.free_node, best_node.region);
    }
    return true;
  }


  //-----------------------
  //       Page tools
  //-----------------------

  static Page* create_page(Pages &pages, const Array<InputRectangle> &rectangles, const i32 edge_max, Indices &indices, Allocator &a)
  {
    Page page;
    page_init(page, a);

    // find starting edge of atlas
    if (array::size(indices) == 1) {
      page.width  = next_pow2_u32(rectangles[indices[0]].width);
      page.height = next_pow2_u32(rectangles[indices[0]].height);
    }
    else {
      i32 perfect_edge;
      i32 min_width = edge_max,
        min_height = edge_max;
      f64 area = 0,
        max_area = pow((f64)edge_max, 2);

      for (u32 i = 0; i < array::size(indices); ++i) {
        if (min_width > rectangles[indices[i]].width) min_width = rectangles[indices[i]].width;
        if (min_height > rectangles[indices[i]].height) min_height = rectangles[indices[i]].height;
        area += rectangles[indices[i]].width * rectangles[indices[i]].height;
        if (max_area <= area) {
          area = max_area;
          break;
        }
      }

      perfect_edge = next_pow2_u32((u32)(sqrt(area)*.5f));

      page.width  = min_width > perfect_edge ? next_pow2_u32(min_width) : perfect_edge;
      page.height = min_height > perfect_edge ? next_pow2_u32(min_height) : perfect_edge;
    }
    page.isfull = 0;

    Region free_node(0, 0, page.width, page.height);
    list::push_back(*page.free_nodes, free_node);

    return &array::push_back(pages, page);
  }

  static void grow_page(Page &page, Indices &indices, const Array<InputRectangle> &rectangles)
  {
    i32 max_region_w = 0,
      max_region_h = 0;

    for (u32 i = 0; i < array::size(indices); i++) {
      const InputRectangle &r = rectangles[indices[i]];
      if (max_region_w < r.width)  max_region_w = r.width;
      if (max_region_h < r.height) max_region_h = r.height;
    }

    // backup input_region_t previously added
    for (u32 i = 0; i < array::size(*page.nodes); i++)
      array::push_back(indices, (*page.nodes)[i].index);

    // clear
    array::clear(*page.nodes);
    list::clear(*page.free_nodes);

    // grow
    if (max_region_h > page.height)
      page.height = next_pow2_u32(max_region_h);
    else if (max_region_w > page.width)
      page.width = next_pow2_u32(max_region_w);
    else if (page.height > page.width)
      page.width = next_pow2_u32(page.width + 1);
    else
      page.height = next_pow2_u32(page.height + 1);

    Region free_node(0, 0, page.width, page.height);

    list::push_back(*page.free_nodes, free_node);
  }


  //-----------------------
  //       Pack tools
  //-----------------------

  static bool pack(Pages &pages, Indices &indices, const Heuristic h, const Array<InputRectangle> &rectangles, i32 edge_max, bool allow_rotation, u32 margin, Allocator &a)
  {
    i32   index;
    Page *page;

    create_page(pages, rectangles, edge_max, indices, a);

    while (array::size(indices) > 0) {
      page = array::begin(pages);

      while (page < array::end(pages)) {
        if (page->isfull) {
          page++;
          continue;
        }
        if (!insert_region(*page, index, rectangles, indices, h, allow_rotation, margin))
          return false;

        if (index != -1) {
          if (index != (i32)array::size(indices) - 1)
            indices[index] = indices[array::size(indices) - 1];

          array::pop_back(indices);
          page = array::end(pages);
          continue;
        }

        //page max size reached
        if (page->height == page->width && page->width == edge_max) {
          if (page == array::end(pages) - 1) {
            page = create_page(pages, rectangles, edge_max, indices, a);
          }
          else {
            page->isfull = 1;
            page++;
          }
        }
        else {
          grow_page(*page, indices, rectangles);
        }
      }
    }
    return true;
  }

  static void shutdown(Array<Pages*> &pages_sets, u32 num_heuristic, Allocator &a)
  {
    for (u32 i = 0; i < num_heuristic; i++) {
      Pages *pages = pages_sets[i];
      result_destroy(*pages, a);
      MAKE_DELETE(a, Pages, pages);
    }
  }
}


namespace pge
{
  namespace texture_packer
  {
    bool process(const Array<InputRectangle> &input_rectangles,
                 Array<PackedRectangle>      &output_rectangles,
                 Array<RectanglePage>        &output_pages,
                 i32  edge_max,
                 bool allow_rotation,
                 u32 spacing,
                 Allocator &a)
    {
      Array<Pages*>  pages_sets(a);
      Indices        indices(a), tmp(a);
      HeuristicScore best_score(0, UINT_MAX, UINT_MAX, UINT_MAX);

      u32 i;
      u32 num_heuristic = HEURISTIC_COUNT;
      u32 edge          = next_pow2_u32(edge_max);

      // init packers result sets
      for (i=0; i < num_heuristic; i++) {
        Pages *pages = MAKE_NEW(a, Pages, a);
        array::push_back(pages_sets, pages);
      }

      // define internal region indices
      array::reserve(indices, array::size(input_rectangles));
      for (i=0; i < array::size(input_rectangles); i++)
        array::push_back(indices, i);

      // generate all pack
      for (i=0; i < num_heuristic; i++) {
        array::copy(tmp, indices);
        if (!pack(*pages_sets[i], tmp, (Heuristic)(i), input_rectangles, edge, allow_rotation, spacing, a)) {
          shutdown(pages_sets, num_heuristic, a);
          return false;
        }
        array::clear(tmp);
      }

      // find best pack solution
      for (i=0; i < num_heuristic; i++)
        compare_result(best_score, i, *pages_sets[i]);

      // set result
      const Pages &best_pages = *pages_sets[best_score.heuristic_index];
      array::resize(output_pages, array::size(best_pages));
      for (u32 i = 0; i < array::size(best_pages); i++) {
        output_pages[i].width  = best_pages[i].width;
        output_pages[i].height = best_pages[i].height;
        output_pages[i].first_rectangle = array::size(output_rectangles);
        output_pages[i].num_rectangles  = array::size(*best_pages[i].nodes);
        array::reserve(output_rectangles, output_pages[i].first_rectangle + output_pages[i].num_rectangles);
        for (u32 j = 0; j < output_pages[i].num_rectangles; j++) {
          PackedRectangle ouput_rect;
          ouput_rect.page  = i;
          ouput_rect.input = (*best_pages[i].nodes)[j].index;
          ouput_rect.rotated = (*best_pages[i].nodes)[j].rotate;
          ouput_rect.x = (*best_pages[i].nodes)[j].region.x;
          ouput_rect.y = (*best_pages[i].nodes)[j].region.y;
          array::push_back(output_rectangles, ouput_rect);
        }
      }

      // clean useless data
      shutdown(pages_sets, num_heuristic, a);

      return true;
    }
  }
}