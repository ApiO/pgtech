#include <string>
#include <vector>
#include <iostream>
#include <windows.h>
#include <mysqlite.h>

#include <engine/pge.h>
#include <runtime/hash.h>
#include <runtime/file_system.h>

#include <controls/list_control.h>
#include "project_handler.h"

using namespace pge;

// win32 specifics

namespace
{
  int SearchDirectory(std::vector<std::string> &refvecFiles,
    const std::string &refcstrRootDirectory,
    const std::string &refcstrExtension,
    bool              bSearchSubdirectories = true)
  {
    std::string     strFilePath;             // Filepath
    std::string     strPattern;              // Pattern
    std::string     strExtension;            // Extension
    HANDLE          hFile;                   // Handle to file
    WIN32_FIND_DATA FileInformation;         // File information

    strPattern = refcstrRootDirectory + "\\*.*";

    hFile = ::FindFirstFile(strPattern.c_str(), &FileInformation);
    if (hFile != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (FileInformation.cFileName[0] != '.')
        {
          strFilePath.erase();
          strFilePath = refcstrRootDirectory + "\\" + FileInformation.cFileName;

          if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          {
            if (bSearchSubdirectories)
            {
              // Search subdirectory
              int iRC = SearchDirectory(refvecFiles,
                strFilePath,
                refcstrExtension,
                bSearchSubdirectories);
              if (iRC)
                return iRC;
            }
          }
          else
          {
            // Check extension
            strExtension = FileInformation.cFileName;
            strExtension = strExtension.substr(strExtension.rfind(".") + 1);

            if (strExtension == refcstrExtension)
            {
              // Save filename
              refvecFiles.push_back(strFilePath);
            }
          }
        }
      } while (::FindNextFile(hFile, &FileInformation) == TRUE);

      // Close handle
      ::FindClose(hFile);

      DWORD dwError = ::GetLastError();
      if (dwError != ERROR_NO_MORE_FILES)
        return dwError;
    }

    return 0;
  }
}


// internal stuff

namespace app
{
  const char strings_query[] = "select id, str from strings;";
  const char units_query[] = "select name from data where type = 5;";
  const char sprites_query[] = "select name from data where type = 6;";

  using namespace controls;
  using namespace handlers;

  inline void load_project_strings(Hash<char*> &strings, sqlite3 *db)
  {
    sqlite3_stmt *stmt_get = NULL;
    sqlite3_check(sqlite3_prepare(db, strings_query, strlen(strings_query) * sizeof(char), &stmt_get, NULL));
    
    Allocator &a = *strings._data._allocator;

    i32 dbr = 0;
    while (dbr != SQLITE_DONE)
    {
      dbr = sqlite3_step(stmt_get);
      if (dbr == SQLITE_DONE) continue;

      XASSERT(dbr == SQLITE_ROW, "query failed.");

      u32   id = (u32)sqlite3_column_int(stmt_get, 0);
      char *name = (char*)(sqlite3_column_text(stmt_get, 1));

      char *str = (char*)a.allocate(strlen(name) * sizeof(char));
      strcpy(str, name);

      hash::set(strings, id, str);
    }
    sqlite3_check(sqlite3_finalize(stmt_get));
  }

  inline void load_data(Hash<char*> &strings, sqlite3 *db, const char *query, Array<char*> &list)
  {
    sqlite3_stmt *stmt_get = NULL;

    sqlite3_check(sqlite3_prepare(db, query, strlen(query) * sizeof(char), &stmt_get, NULL));

    i32 dbr = 0;
    while (dbr != SQLITE_DONE)
    {
      dbr = sqlite3_step(stmt_get);
      if (dbr == SQLITE_DONE) continue;

      XASSERT(dbr == SQLITE_ROW, "query failed.");

      u32 id = (u32)sqlite3_column_int(stmt_get, 0);

      array::push_back(list, *hash::get(strings, id));

    }
    sqlite3_check(sqlite3_finalize(stmt_get));
  }

  /*
  inline void destroy_list(Allocator &a, ListControl &list)
  {
    for (u32 i = 0; i < list.size(); i++)
      a.deallocate(list[i].data);

    list.clear();
  }
  */
}

// privates

namespace app
{
  namespace handlers
  {
    void ProjectHandler::load_levels_data()
    {
      std::vector<std::string> strings;

      int res = SearchDirectory(strings, src_path, "pglevel", true);

      ASSERT(!res);
      const char *name = NULL;
      u32 i = 0u, len;

      array::resize(levels, strings.size());

      Allocator &a = *units._allocator;

      for (std::vector<std::string>::iterator iterTxt = strings.begin();
        iterTxt != strings.end();
        ++iterTxt)
      {
        name = (*iterTxt).c_str() + strlen(src_path) + 1;
        len = strlen(name) - 8;

        levels[i] = (char*)a.allocate(len + 1);
        memcpy(levels[i], name, len);
        levels[i][len] = '\0';
        i++;
      }
    }
  }
}


// Header's definition

namespace app
{
  namespace handlers
  {
    ProjectHandler *project;

    namespace project_handler
    {
      void init(Allocator &a)
      {
        project = MAKE_NEW(a, ProjectHandler, a);
      }

      void shutdown(Allocator &a)
      {
        if (project->get_source_path())
          project->close();

        MAKE_DELETE(a, ProjectHandler, project);
        project = NULL;
      }

    }

    void ProjectHandler::load(const char *src, const char *data)
    {
      if (src) close();

      Allocator &a = *units._allocator;

      data_path = (char*)a.allocate(strlen(data) * sizeof(char));
      strcpy(data_path, data);

      src_path = (char*)a.allocate(strlen(src) * sizeof(char));
      strcpy(src_path, src);


      char szdb[_MAX_PATH];
      sprintf(szdb, "%s\\%s", data_path, "build.db");

      ASSERT(file_system::file_exists(szdb));

      sqlite3 *db = NULL;

      sqlite3_check(sqlite3_open(szdb, &db));

      load_project_strings(strings, db);
      load_levels_data();
      load_data(strings, db, units_query, units);
      load_data(strings, db, sprites_query, sprites);

      sqlite3_check(sqlite3_close(db));
    }

    void ProjectHandler::close(void)
    {
      Allocator &a = *units._allocator;

      // cleans project levels
      for (u32 i = 0; i < array::size(levels); i++)
        a.deallocate(levels[i]);
      array::clear(levels);

      // clears units/sprites list
      array::clear(units);
      array::clear(sprites);

      // cleans strings hash
      const Hash<char*>::Entry *e, *end = hash::end(strings);
      for (e = hash::begin(strings); e < end; e++)
        a.deallocate(e->value);
      hash::clear(strings);

      a.deallocate(src_path);
      a.deallocate(data_path);

      data_path = NULL;
      src_path  = NULL;
    }

    const char *ProjectHandler::get_source_path(void)
    {
      return src_path;
    }

    const char *ProjectHandler::get_data_path(void)
    {
      return data_path;
    }

    const char *ProjectHandler::get_string(pge::u32 key)
    {
      return *hash::get(strings, key);
    }

    const Array<char*> &ProjectHandler::levels_strings(void)
    {
      return levels;
    }

    const Array<char*> &ProjectHandler::units_strings(void)
    {
      return units;
    }

    const Array<char*> &ProjectHandler::sprites_strings(void)
    {
      return sprites;
    }

    void ProjectHandler::add_level(const char *name)
    {
      bool add = true;
      for (u32 i = 0; i < array::size(levels); i++){
        if (strcmp(levels[i], name) == 0){
          add = false;
          break;
        }
      }

      if (add){
        char *data = (char*)units._allocator->allocate(strlen(name) + 1);
        strcpy((char*)data, name);
        array::push_back(levels, data);
      }
    }

    void ProjectHandler::remove_level(const char *name)
    {
      Allocator &a = *units._allocator;
      for (u32 i = 0; i < array::size(levels); i++){
        if (strcmp(levels[i], name) == 0){
          a.deallocate(levels[i]);
          levels[i] = array::pop_back(levels);
          return;
        }
      }
    }
  }
}