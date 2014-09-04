#pragma once

#include <glm/glm.hpp>

#include "audio_types.h"

namespace pge
{
  namespace audio_system
  {
    // Initializes the audio system with the specified configuration.
    void init(u32 config_name, Allocator &a);

    // Updates the audio system.
    void update(AudioWorld &world);

    // Triggers the sound with the specified name at position.
    u64 trigger_sound(AudioWorld &world, const char *name, const glm::vec3 &position);

    // Triggers the sound with the specified name.
    // The sound will be ambient since there is no position specified.
    u64 trigger_sound(AudioWorld &world, const char *name);

    // Sets the position of a playing sound.
    // If the sound is ambient, it will be added to the world.
    void set_position(AudioWorld &world, u64 playing_sound, const glm::vec3 &position);

    // Stops the playing sound with the specified id.
    void stop(AudioWorld &world, u64 playing_sound);

    // Stops all the sounds of the specified world.
    void stop_all(AudioWorld &world);

    // Sets the position of the listener for the specified world.
    void set_listener(AudioWorld &world, const glm::vec3 &position);

    // Sets the volume of the bus with the specified name
    void set_bus_volume(const char *name, f32 volume);

    // Sets the pan of the bus with the specified name
    void set_bus_pan(const char *name, f32 pan);
    
    // Shutdowns the audio system
    void shutdown();
  }
}