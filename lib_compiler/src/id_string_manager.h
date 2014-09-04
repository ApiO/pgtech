#pragma once

#include <data/types.h>
#include "id_string_types.h"

namespace pge
{
  namespace id_string_manager
  {
    void  startup  (IdStringManager &mng, sqlite3 *db);
    u32   create   (IdStringManager &mng, const char *str);
    bool  has      (IdStringManager &mng, u32 id);
    char *lookup   (IdStringManager &mng, u32 id);
    void  save     (IdStringManager &mng);
    void  shutdown (IdStringManager &mng);
  }
}

namespace pge
{
  inline IdStringManager::IdStringManager(Allocator &a) :
    _allocator(&a), _sqlbuf(a), _id_strings(a), _to_save(a) {};
}
