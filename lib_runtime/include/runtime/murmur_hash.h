#pragma once

#include "runtime/types.h"

namespace pge
{
	/// Implementation of the MurmurHash2 functions
	/// http://murmurhash.googlepages.com/

  u32 murmur_hash_32 (const void *key, u32 seed, u32 len);
  u32 murmur_hash_32 (const char *key, u32 seed = 0);

  u64 murmur_hash_64 (const void *key, u64 seed, u32 len);
  u64 murmur_hash_64 (const char *key, u64 seed = 0);
}