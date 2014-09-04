#include <engine/pge.h>

#include "audio/audio_system.h"
#include "application.h"

namespace pge
{
  namespace audio
  {
    // Plays the sound resource name as a 2D sound.
    u64 trigger_sound(u64 world, const char *name)
    {
      World &w = application::world(world);
      return audio_system::trigger_sound(w.audio_world, name);
    }

    // Plays the sound resource name in the world and returns an id to it. 
    // u64 trigger_sound(World world, const char *name, const glm::vec3 &position);

    // Plays the sound resource name in the world and returns an id to it. 
    // Links the position of the sound to the position of the node in the unit.
    // u64 trigger_sound(World world, const char *name, Unit unit, i32 node);

    void stop(u64 world, u64 id) 
    {
      World &w = application::world(world);
      audio_system::stop(w.audio_world, id);
    }

    // Stops all sounds playing in the world.
    void stop_all(u64 world) 
    {
      World &w = application::world(world);
      audio_system::stop_all(w.audio_world);
    }

    // Sets the audio listener to the specified position
    // void set_listener(World world, const glm::vec3 &position);

    // Sets the volume of the specified bus.
    // 1.0 means full strength, 0.0 means it's muted.
    // void set_bus_volume(World world, const char *name, f32 volume);

    // Sets the reverb environment for sounds.
    // void set_environment(World world, const char *name);
  }
}