#include <string.h>

#include "runtime/murmur_hash.h"

namespace pge
{
  u32 murmur_hash_32(const char *key, u32 seed)
  {
    return murmur_hash_32(key, seed, strlen(key));
  }

  u32 murmur_hash_32(const void *key, u32 seed, u32 len)
  {
    const u32 m = 0x5bd1e995U;
    const u32 r = 24;

    u32 h = seed ^ len;

    const u8 *data = (const u8*)key;

    while(len >= 4)
    {
      u32 k = *(u32*)data;
      #ifdef PLATFORM_BIG_ENDIAN
        k = swap_u64(k);
      #endif

      k *= m; 
      k ^= k >> r; 
      k *= m; 

      h *= m; 
      h ^= k;

      data += 4;
      len -= 4;
    }

    switch(len)
    {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
      h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  } 

  u64 murmur_hash_64(const char *key, u64 seed)
  {
    return murmur_hash_64(key, seed, strlen(key));
  }

  u64 murmur_hash_64(const void *key, u64 seed, u32 len)
  {
    const u64 m = 0xc6a4a7935bd1e995ULL;
    const u32 r = 47;

    u64 h = seed ^ (len * m);

    const u64 *data = (const u64*)key;
    const u64 *end = data + (len/8);

    while(data != end)
    {
      u64 k = *data++;
      #ifdef PLATFORM_BIG_ENDIAN
        k = swap_u64(k);
      #endif

      k *= m;
      k ^= k >> r;
      k *= m;

      h ^= k;
      h *= m;
    } {
      const u8 *data2 = (const u8*)data;
      switch(len & 7)
      {
      case 7: h ^= ((u64)data2[6]) << 48;
      case 6: h ^= ((u64)data2[5]) << 40;
      case 5: h ^= ((u64)data2[4]) << 32;
      case 4: h ^= ((u64)data2[3]) << 24;
      case 3: h ^= ((u64)data2[2]) << 16;
      case 2: h ^= ((u64)data2[1]) << 8;
      case 1: h ^= ((u64)data2[0]);
        h *= m;
      };
    }

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
  }
}