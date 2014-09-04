#include <runtime/types.h>
#include <runtime/trace.h>

#include <data/audio.h>

#include "compiler_types.h"
#include "compiler.h"

#include <algorithm>

namespace
{
  using namespace pge;

  struct BusKey {
    u64 node;
    u32 name;
    u32 parent_name;
    i32 parent_index;
  };

  bool make_bus_keys(Work &w, const Json &doc, u64 buses, Array<BusKey> &bus_keys)
  {
    const u32 num_buses = json::size(doc, buses);
    array::resize(bus_keys, num_buses);
    i32 iroot = -1;

    for (u32 i = 0; i < num_buses; i++) {
      const Json::Node &n = json::get_node(doc, buses, i);
      bus_keys[i].node = n.id;
      bus_keys[i].name = compiler::create_id_string(w, n.name);

      const char *parent = json::get_string(doc, n.id, "parent", NULL);

      if (parent) {
        if (!json::has(doc, buses, parent)){
          LOG("The bus named \"%s\" could not be found.", parent);
          return false;
        }
      } else {
        if (iroot >= 0){
          LOG("Only one root can be defined. Please attach the \"%s\" bus to another bus.", n.name);
          return false;
        }
        iroot = i;
        parent = "_root";
      }
      bus_keys[i].parent_name  = compiler::create_id_string(w, parent);
      bus_keys[i].parent_index = -1;
    }

    if (iroot != 0) { // swap the root with the first node
      BusKey tmp = bus_keys[0];
      bus_keys[0] = bus_keys[iroot];
      bus_keys[iroot] = tmp;
    }
    return true;
  }

  void sort_bus_keys(Array<BusKey> &bus_keys)
  {
    const u32 num_buses = array::size(bus_keys);
    u32 num_sorted = 1; // root is already first

    for (u32 i = 0; i < num_buses; i++) {
      u32 parent_name = bus_keys[i].name;

      // skip already sorted nodes
      while (bus_keys[num_sorted].parent_name == parent_name && num_sorted < num_buses) {
        bus_keys[num_sorted].parent_index = i;
        ++num_sorted;
      }

      for (u32 child = num_sorted; child < num_buses; child++) {
        if (bus_keys[child].parent_name == parent_name) {
          BusKey tmp  = bus_keys[num_sorted];
          bus_keys[num_sorted] = bus_keys[child];
          bus_keys[num_sorted].parent_index = i;
          bus_keys[child] = tmp;
          ++num_sorted;
        }
      }
    }
  }
}

namespace pge
{
  AudioCompiler::AudioCompiler(Allocator &a, StringPool &sp)
    : JsonCompiler(a, RESOURCE_TYPE_AUDIO, sp) {}

  bool AudioCompiler::compile(Work &w)
  {
    AudioResource ar;
    Array<BusKey> bus_keys(*a);
    Array<u32> bus_names(*a);
    Array<AudioResource::Bus> buses(*a);

    if (!load_json(w)) return false;

    const u64 buses_id = json::get_id(jsn, json::root(jsn), "buses");

    ar._sampling_rate = json::get_integer(jsn, json::root(jsn), "sampling_rate", 44100);
    ar._buffer_size = json::get_integer(jsn, json::root(jsn), "buffer_size", 4096);
    ar._num_buses = json::size(jsn, buses_id);
    ar._flags = 0;

    if (json::get_bool(jsn, json::root(jsn), "roundoff_clipping", true))
      ar._flags |= AudioResource::Flags::ROUNDOFF_CLIPPING;

    if (json::get_bool(jsn, json::root(jsn), "enable_vizualisation", false))
      ar._flags |= AudioResource::Flags::ENABLE_VISUALIZATION;

    // write header
    fwrite(&ar, sizeof(AudioResource), 1, w.data);

    // make & sort bus keys by parent first
    if(!make_bus_keys(w, jsn, buses_id, bus_keys)) return false;
    sort_bus_keys(bus_keys);

    // populate buses regarding the order
    array::resize(bus_names, ar._num_buses);
    array::resize(buses, ar._num_buses);

    for (i32 i = 0; i < ar._num_buses; i++) {
      const Json::Node &bus = json::get_node(jsn, bus_keys[i].node);
      bus_names[i] = bus_keys[i].name;
      buses[i].parent = bus_keys[i].parent_index;
      buses[i].volume = (f32)json::get_number(jsn, bus.id, "volume", 1.0);
      buses[i].pan    = (f32)json::get_number(jsn, bus.id, "pan", 0.0);
    }

    // write bus data
    fwrite(array::begin(bus_names), sizeof(u32), array::size(bus_names), w.data);
    fwrite(array::begin(buses), sizeof(AudioResource::Bus), array::size(buses), w.data);

    return true;
  }
}