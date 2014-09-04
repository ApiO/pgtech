#include <runtime/file_system.h>

#include <windows.h>

namespace pge
{
  namespace file_system
  {
    int delete_file(const char *path)
    {
      if (DeleteFile(path) == FALSE)
        return ::GetLastError();

      return 0;
    }

    bool file_exists(const char *path)
    {
      return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
    }

    void copy_file(const char *dest, const char *source)
    {
      CopyFile(source, dest, false);
    }
  }
}