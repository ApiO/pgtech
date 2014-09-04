#pragma once

#include <runtime/memory.h>
#include <runtime/collection_types.h>
#include <runtime/array.h>

namespace app
{
  using namespace pge;

  namespace handlers
  {
    class ProjectHandler
    {
    public:
      ProjectHandler(Allocator &_a);

      void load  (const char *src_path, const char *data_path);
      void close (void);

      void add_level    (const char *name);
      void remove_level (const char *name);

      const char * get_string (u32 key);

      const Array<char*> &levels_strings  (void);
      const Array<char*> &units_strings   (void);
      const Array<char*> &sprites_strings (void);

      const char *get_source_path (void);
      const char *get_data_path   (void);
    private:
      bool  reload;
      char *src_path;
      char *data_path;
      Hash<char*>  strings;
      Array<char*> levels;
      Array<char*> units;
      Array<char*> sprites;

      void load_levels_data();
    };

    inline ProjectHandler::ProjectHandler(Allocator &_a)
      : levels(_a), units(_a), sprites(_a), src_path(NULL), data_path(NULL), strings(_a){}

    namespace project_handler
    {
      void init(Allocator &a);
      void shutdown(Allocator &a);
    }

    extern ProjectHandler *project;
  }
}