#include <string.h>

#include "runtime/murmur_hash.h"
#include "runtime/hash.h"

namespace pge
{
  namespace string_pool
  {
    char *acquire(StringPool &pool, const char *str, u32 n)
    {
      const u64 key = murmur_hash_64(str, 0, n);
      StringPool::Entry e = {0};
      e = hash::get(pool._map, key, e);

      if (e.str) {
        ++e.refs;
      } else {
        e.refs = 1;
        e.str  = (char*)pool._allocator->allocate(n + 1);
        memcpy(e.str, str, n);
        e.str[n] = '\0';
        hash::set(pool._pointer_to_key, (u64)e.str, key);
      }

      hash::set(pool._map, key, e);
      return e.str;
    }

    void release(StringPool &pool, const char *str)
    {
      const u64 key = hash::get(pool._pointer_to_key, (u64)str, key);

      StringPool::Entry e = {0};
      e = hash::get(pool._map, key, e);

      if (!e.str)
        return;

      if (--e.refs > 0) {
        hash::set(pool._map, key, e);
      } else {
        hash::remove(pool._pointer_to_key, (u64)str);
        pool._allocator->deallocate(e.str);
        hash::remove(pool._map, key);
      }
    }

    char *acquire(StringPool &pool, const char *str)
    {
      return acquire(pool, str, strlen(str));
    }

    bool has(StringPool &pool, const char *str)
    {
      return hash::has(pool._map, murmur_hash_64(str));
    }
  }

  StringPool::StringPool(Allocator &a) : _allocator(&a), _map(a), _pointer_to_key(a) {}

  StringPool::~StringPool()
  {
    const Hash<Entry>::Entry *e   = hash::begin(_map);
    const Hash<Entry>::Entry *end = hash::end  (_map);

    while(e < end)
    {
      _allocator->deallocate(e->value.str);
      ++e;
    }
  }
}