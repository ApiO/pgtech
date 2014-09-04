#include <runtime/types.h>
#include <runtime/temp_allocator.h>
#include <runtime/trace.h>

#include <data/unit.h>
#include <data/mover.h>

#include "compiler_types.h"
#include "compiler.h"

namespace
{
  using namespace pge;

  struct NodeKey
  {
    u64 node;
    u32 name;
    u32 parent_name;
    i32 parent_index;
  };

  struct AnimsetData
  {
    struct SpriteData {
      u32 index;
      u32 first_frame;
      u32 num_frames;
    };

    AnimsetData(Allocator &a) : bone_name_to_index(a), sprite_data(a), frame_names(a) {}
    Hash<u32>        bone_name_to_index;
    Hash<SpriteData> sprite_data;
    Array<u32>       frame_names;
  };

  // make the sort keys and place the root first (node without parent)
  bool make_node_keys(Work &w, const Json &input, u64 nodes, Array<NodeKey> &node_keys)
  {
    const u32 num_nodes = json::size(input, nodes);
    array::resize(node_keys, num_nodes);
    i32 iroot = -1;

    for (u32 i = 0; i < num_nodes; i++) {
      const Json::Node &n = json::get_node(input, nodes, i);
      node_keys[i].node   = n.id;
      node_keys[i].name   = compiler::create_id_string(w, n.name);

      const char *parent = json::get_string(input, n.id, "parent", NULL);

      if (parent) {
        if (!json::has(input, nodes, parent)){
          LOG("The node named \"%s\" could not be found", parent);
          return false;
        }
      }
      else {
        if (iroot >= 0){
          LOG("Only one root can be defined. Please attach the \"%s\" node to another node", n.name);
          return false;
        }
        iroot = i;
        parent = "root";
      }
      node_keys[i].parent_name  = compiler::create_id_string(w, parent);
      node_keys[i].parent_index = -1;
    }

    if (iroot != 0) { // swap the root with the first node
      NodeKey tmp = node_keys[0];
      node_keys[0] = node_keys[iroot];
      node_keys[iroot] = tmp;
    }
    return true;
  }

  void sort_node_keys(Array<NodeKey> &node_keys)
  {
    const u32 num_nodes = array::size(node_keys);
    u32 num_sorted = 1; // root is already first

    for (u32 i = 0; i < num_nodes; i++) {
      u32 parent_name = node_keys[i].name;
      u32 parent_index = i;

      // skip already sorted nodes
      while (node_keys[num_sorted].parent_name == parent_name && num_sorted < num_nodes) {
        node_keys[num_sorted].parent_index = parent_index;
        ++num_sorted;
      }

      for (u32 child = num_sorted; child < num_nodes; child++) {
        if (node_keys[child].parent_name == parent_name) {
          NodeKey tmp  = node_keys[num_sorted];
          node_keys[num_sorted] = node_keys[child];
          node_keys[num_sorted].parent_index = parent_index;
          node_keys[child] = tmp;
          ++num_sorted;
        }
      }
    }
  }

  void load_animset_data(Work &w, Json &src, const char *animset_name, AnimsetData &data)
  {
    Allocator &a = *data.sprite_data._data._allocator;
    Json animset_doc(a, *src._string_pool);
    compiler::load_dependency(w, RESOURCE_TYPE_ANIMSET, animset_name, animset_doc);

    if (!json::has(animset_doc, json::root(animset_doc), "tracks"))
      return;

    const u64 tracks = json::get_id(animset_doc, json::root(animset_doc), "tracks");

    if (json::has(animset_doc, tracks, "bone")) {
      const u64 bone_tracks = json::get_id(animset_doc, tracks, "bone");
      const u32 num_bone_tracks = (u32)json::size(animset_doc, bone_tracks);
      for (u32 i = 0; i < num_bone_tracks; i++)
        hash::set(data.bone_name_to_index, compiler::create_id_string(w, json::get_node(animset_doc, bone_tracks, i).name), i);
    }

    if (json::has(animset_doc, tracks, "sprite")) {
      const u64 sprite_tracks = json::get_id(animset_doc, tracks, "sprite");
      const u32 num_sprite_tracks = (u32)json::size(animset_doc, sprite_tracks);
      for (u32 i = 0; i < num_sprite_tracks; i++) {
        const Json::Node &sn = json::get_node(animset_doc, sprite_tracks, i);
        AnimsetData::SpriteData sd;
        sd.index = i;
        sd.num_frames = json::size(animset_doc, sn.id);
        sd.first_frame = array::size(data.frame_names);
        hash::set(data.sprite_data, compiler::create_id_string(w, sn.name), sd);
        array::resize(data.frame_names, sd.first_frame + sd.num_frames);
        for (u32 j = 0; j < sd.num_frames; j++)
          data.frame_names[sd.first_frame + j] = compiler::create_id_string(w, json::get_string(animset_doc, sn.id, j));
      }
    }
  }


  bool write_nodes(Work &w, const Json &input, u64 nodes, u32 num_nodes, Hash<u32> &node_name_to_index)
  {
    TempAllocator256 ta;
    Array<NodeKey>   node_keys(ta);
    UnitResource::Node rn;

    if (!make_node_keys(w, input, nodes, node_keys)) return false;

    sort_node_keys(node_keys); // put the parent before the children and set parent_index

    for (u32 i = 0; i < num_nodes; i++) {
      const NodeKey &nkey = node_keys[i];
      const Json::Node &n = json::get_node(input, nkey.node);
      rn.name = nkey.name;
      rn.parent = nkey.parent_index;
      compiler::read_json_pose(input, n.id, rn.pose);

      fwrite(&rn, sizeof(UnitResource::Node), 1, w.data);
      hash::set(node_name_to_index, nkey.name, i);
    }
    return true;
  }

  bool write_sprites(Work &w, const Json &jsn, u64 sprites, u32 num_sprites, const Hash<u32> &node_name_to_index, Hash<u32> &sprite_name_to_index)
  {
    UnitResource::Sprite s;

    for (u32 i = 0; i < num_sprites; i++) {
      const Json::Node &node = json::get_node(jsn, sprites, i);
      s.name = compiler::create_id_string(w, node.name);
      s.tpl = compiler::create_id_string(w, json::get_string(jsn, node.id, "template"));
      s.node = hash::get(node_name_to_index, (u64)compiler::create_id_string(w, json::get_string(jsn, node.id, "node")), UINT_MAX);
      s.order = json::get_integer(jsn, node.id, "order", 0);
      compiler::read_json_color(jsn, node.id, s.color, "color");
      compiler::read_json_pose(jsn, node.id, s.pose);

      if (s.node == UINT_MAX) {
        LOG("could not find the node named %s.", json::get_string(jsn, node.id, "node"));
        return false;
      }

      fwrite(&s, sizeof(UnitResource::Sprite), 1, w.data);
      hash::set(sprite_name_to_index, (u64)s.name, i);
    }
    return true;
  }

  void write_actors(Work &w, const Json &jsn, u64 actors, u32 num_actors, const Hash<u32> &node_name_to_index)
  {
    UnitResource::Actor actor;

    for (u32 i = 0; i < num_actors; i++) {
      const Json::Node &node = json::get_node(jsn, actors, i);
      actor.instance_name = compiler::create_id_string(w, node.name);
      actor.actor = compiler::create_id_string(w, json::get_string(jsn, node.id, "actor"));
      actor.node = hash::get(node_name_to_index, (u64)compiler::create_id_string(w, json::get_string(jsn, node.id, "node")), UINT_MAX);
      compiler::read_json_pose(jsn, node.id, actor.pose);
      fwrite(&actor, sizeof(UnitResource::Actor), 1, w.data);
    }
  }

  void write_movers(Work &w, const Json &jsn, u64 movers, u32 num_movers)
  {
    MoverResource m;

    for (u32 i = 0; i < num_movers; i++) {
      const Json::Node &n = json::get_node(jsn, movers, i);
      m.name        = compiler::create_id_string(w, n.name);
      m.height      = (f32)json::get_number(jsn, n.id, "height");
      m.radius      = (f32)json::get_number(jsn, n.id, "radius");
      m.slope_limit = (f32)json::get_number(jsn, n.id, "slope_limit");
      m.step_offset = (f32)json::get_number(jsn, n.id, "step_offset");
      m.collision_filter = json::has(jsn, n.id, "collision_filter")
        ? compiler::create_id_string(w, json::get_string(jsn, n.id, "collision_filter")) : 0;

      m.offset[0] = 0;
      m.offset[1] = 0;

      if (json::has(jsn, n.id, "offset")) {
        const u64 offset = json::get_id(jsn, n.id, "offset");
        m.offset[0] = (f32)json::get_number(jsn, offset, 0);
        m.offset[1] = (f32)json::get_number(jsn, offset, 1);
      }

      fwrite(&m, sizeof(MoverResource), 1, w.data);
    }
  }

  bool write_bone_tracks(Work &w, FILE *stream, const Json &jsn, u64 bone_tracks, u32 num_bone_tracks, const Hash<u32> & node_name_to_index, AnimsetData &animset_data)
  {
    for (u32 i = 0; i < num_bone_tracks; i++) {
      const Json::Node &node = json::get_node(jsn, bone_tracks, i);
      UnitResource::BoneTrack bt;
      bt.node  = hash::get(node_name_to_index, (u64)compiler::create_id_string(w, node.name), UINT_MAX);
      bt.track = hash::get(animset_data.bone_name_to_index, compiler::create_id_string(w, node.value.string), UINT_MAX);

      if (bt.node == UINT_MAX) {
        LOG("could not find the node named %s.", node.name);
        return false;
      }

      if (bt.track == UINT_MAX) {
        LOG("could not find the bone track named %s.", node.value.string);
        return false;
      }

      fwrite(&bt, sizeof(UnitResource::BoneTrack), 1, stream);
    }
    return true;
  }

  bool write_sprite_tracks(Work &w, FILE *stream, const Json &jsn, u64 sprite_tracks, u32 num_sprite_tracks, const Hash<u32> & sprite_name_to_index, AnimsetData &animset_data)
  {
    u32 frame_offset = ftell(stream) + num_sprite_tracks * sizeof(UnitResource::SpriteTrack);

    // write sprite tracks
    for (u32 i = 0; i < num_sprite_tracks; i++) {
      const Json::Node &node = json::get_node(jsn, sprite_tracks, i);
      const u32 track_name = compiler::create_id_string(w, json::get_string(jsn, node.id, "track"));
      UnitResource::SpriteTrack st;
      st.sprite = hash::get(sprite_name_to_index, compiler::create_id_string(w, node.name), UINT_MAX);
      st.track = hash::has(animset_data.sprite_data, track_name) ? hash::get(animset_data.sprite_data, track_name)->index : UINT_MAX;
      st.frame_offset = frame_offset;

      if (st.sprite == UINT_MAX) {
        LOG("could not find the sprite named %s.", node.name);
        return false;
      }

      if (st.track == UINT_MAX) {
        LOG("could not find the sprite track named %s.", json::get_string(jsn, node.id, "track"));
        return false;
      }

      fwrite(&st, sizeof(UnitResource::SpriteTrack), 1, stream);

      const u32 num_frames = json::has(jsn, node.id, "frames") ? json::size(jsn, node.id, "frames") : 0;
      frame_offset += num_frames * sizeof(u16);
    }

    TempAllocator512 ta;
    Hash<u16> sf(ta);

    // write sprite track frames
    for (u32 i = 0; i < num_sprite_tracks; i++) {
      const Json::Node &node = json::get_node(jsn, sprite_tracks, i);
      const AnimsetData::SpriteData *sd = hash::get(animset_data.sprite_data, compiler::create_id_string(w, json::get_string(jsn, node.id, "track")));

      const u64 sprite_frames = json::has(jsn, node.id, "frames") ? json::get_id(jsn, node.id, "frames") : json::NO_NODE;
      const u32 num_frames = sprite_frames != json::NO_NODE ? json::size(jsn, sprite_frames) : 0;

      hash::reserve(sf, num_frames);
      for (u16 j = 0; j < num_frames; j++)
        hash::set(sf, compiler::create_id_string(w, json::get_string(jsn, sprite_frames, j)), j);

      for (u32 j = 0; j < sd->num_frames; j++) {
        u16 frame = hash::get(sf, animset_data.frame_names[sd->first_frame + j], NO_FRAME);
        fwrite(&frame, sizeof(u16), 1, stream);
      }
      hash::clear(sf);
    }
    return true;
  }
}

namespace pge
{
  UnitCompiler::UnitCompiler(Allocator &a, StringPool &sp)
    : JsonCompiler(a, RESOURCE_TYPE_UNIT, sp) {}

  bool UnitCompiler::compile(Work &w)
  {
    if (!load_json(w)) return false;

    Hash<u32> node_name_to_index(*a);
    Hash<u32> sprite_name_to_index(*a);

    const u64 root          = json::root(jsn);
    const u64 nodes         = json::get_id(jsn, root, "nodes");
    const u64 actors        = json::has(jsn, root, "actors") ? json::get_id(jsn, root, "actors") : json::NO_NODE;
    const u64 sprites       = json::has(jsn, root, "sprites") ? json::get_id(jsn, root, "sprites") : json::NO_NODE;
    const u64 movers        = json::has(jsn, root, "movers") ? json::get_id(jsn, root, "movers") : json::NO_NODE;
    const u64 animations    = json::has(jsn, root, "animations") ? json::get_id(jsn, root, "animations") : json::NO_NODE;
    const u64 bone_tracks   = json::has(jsn, animations, "bones") ? json::get_id(jsn, animations, "bones") : json::NO_NODE;
    const u64 sprite_tracks = json::has(jsn, animations, "sprites") ? json::get_id(jsn, animations, "sprites") : json::NO_NODE;

    UnitResource unit;
    unit.num_nodes         = nodes == json::NO_NODE ? 0 : json::size(jsn, nodes);
    unit.num_actors        = actors == json::NO_NODE ? 0 : json::size(jsn, actors);
    unit.num_sprites       = sprites == json::NO_NODE ? 0 : json::size(jsn, sprites);
    unit.num_movers        = movers == json::NO_NODE ? 0 : json::size(jsn, movers);
    unit.num_bone_tracks   = bone_tracks == json::NO_NODE ? 0 : (u16)json::size(jsn, bone_tracks);
    unit.num_sprite_tracks = sprite_tracks == json::NO_NODE ? 0 : (u16)json::size(jsn, sprite_tracks);
    unit.animation_set     = animations == json::NO_NODE ? 0 : compiler::create_id_string(w, json::get_string(jsn, animations, "set"));

    unit._actor_offset        = sizeof(UnitResource)+(sizeof(UnitResource::Node) * unit.num_nodes);
    unit._sprite_offset       = unit._actor_offset + (sizeof(UnitResource::Actor) * unit.num_actors);
    unit._mover_offset        = unit._sprite_offset + (sizeof(UnitResource::Sprite) * unit.num_sprites);
    unit._bone_track_offset   = unit._mover_offset + (sizeof(MoverResource)* unit.num_movers);
    unit._sprite_track_offset = unit._bone_track_offset + (sizeof(UnitResource::BoneTrack) * unit.num_bone_tracks);

    fwrite(&unit, sizeof(UnitResource), 1, w.data);

    if (!write_nodes(w, jsn, nodes, unit.num_nodes, node_name_to_index))
      return false;

    write_actors(w, jsn, actors, unit.num_actors, node_name_to_index);

    if (!write_sprites(w, jsn, sprites, unit.num_sprites, node_name_to_index, sprite_name_to_index))
      return false;

    /*
    if (strstr(w.src, "spineboy.pgunit"))
    {
      u32 index = *hash::get(node_name_to_index, compiler::create_id_string(w, "left_hand"));
      printf("%u\n", index);

      index = *hash::get(node_name_to_index, compiler::create_id_string(w, "right_hand"));
      printf("%u\n", index);
    }
    //*/

    write_movers(w, jsn, movers, unit.num_movers);

    if (!unit.animation_set)
      return true;

    {
      AnimsetData ad(*a);
      load_animset_data(w, jsn, json::get_string(jsn, animations, "set"), ad);

      if (!write_bone_tracks(w, w.data, jsn, bone_tracks, unit.num_bone_tracks, node_name_to_index, ad)) return false;
      if (!write_sprite_tracks(w, w.data, jsn, sprite_tracks, unit.num_sprite_tracks, sprite_name_to_index, ad)) return false;
    }

    return true;
  }
}