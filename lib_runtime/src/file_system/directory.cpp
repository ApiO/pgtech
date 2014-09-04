#include <runtime/file_system.h>

#include <windows.h>
#include <stdio.h>

namespace pge
{
  namespace file_system
  {
    int delete_directory(const char *path, bool recursive)
    {
      bool            has_sub_dir = false; // Flag, indicating whether
      // subdirectories have been found
      HANDLE          hFile;               // Handle to directory
      char            pattern[MAX_PATH];   // Pattern
      WIN32_FIND_DATA file_info;           // File information

      sprintf(pattern, "%s\\*.*", path);

      hFile = ::FindFirstFile(pattern, &file_info);
      if (hFile != INVALID_HANDLE_VALUE)
      {
        int r = delete_directory_content(path, recursive);
        if (r != 0) return r;

        DWORD dwError = ::GetLastError();
        if (dwError != ERROR_NO_MORE_FILES)
          return dwError;
        else
        {
          if (!has_sub_dir)
          {
            // Set directory attributes
            if (::SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL) == FALSE)
              return ::GetLastError();

            // Delete directory
            if (::RemoveDirectory(path) == FALSE)
              return ::GetLastError();
          }
        }
      }
      return 0;
    }

    int delete_directory_content(const char *path, bool recursive)
    {
      bool has_sub_dir = false; // Flag, indicating whether
      // subdirectories have been found
      HANDLE          hFile;               // Handle to directory
      char            file_path[MAX_PATH]; // Filepath
      char            pattern[MAX_PATH];   // Pattern
      WIN32_FIND_DATA file_info;           // File information

      sprintf(pattern, "%s\\*.*", path);

      hFile = ::FindFirstFile(pattern, &file_info);
      if (hFile != INVALID_HANDLE_VALUE)
      {
        do
        {
          if (file_info.cFileName[0] != '.')
          {
            sprintf(file_path, "%s\\%s", path, file_info.cFileName);

            if (file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
              if (recursive)
              {
                // Delete subdirectory
                int iRC = delete_directory(file_path, recursive);
                if (iRC)
                  return iRC;
              }
              else
                has_sub_dir = true;
            }
            else
            {
              // Set file attributes
              if (::SetFileAttributes(file_path, FILE_ATTRIBUTE_NORMAL) == FALSE)
                return ::GetLastError();

              // Delete file
              if (::DeleteFile(file_path) == FALSE)
                return ::GetLastError();
            }
          }
        } while (::FindNextFile(hFile, &file_info) == TRUE);

        // Close handle
        ::FindClose(hFile);
      }
      return 0;
    }

    bool directory_exists(const char *path)
    {
      DWORD dwAttrib = GetFileAttributes(path);

      return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
              (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
    }

    bool directory_create(const char *path)
    {
      return CreateDirectory(path, NULL) || ERROR_ALREADY_EXISTS == GetLastError();
    }
  }
}