#include <runtime/types.h>
#include <runtime/array.h>

#include <data/data_manager.h>

namespace pge
{
  namespace data_manager
  {
    using namespace string_stream;

    void startup(DataManager &ctx, const char *data_dir, sqlite3 *db)
    {
      strcpy(ctx.dir, data_dir);
      ctx.db = db;

      { // Create the tables.
        const char sql_create_tables[] =
          "CREATE TABLE IF NOT EXISTS data ("
          "  type  UNSIGNED INT,"
          "  name  UNSIGNED INT,"
          "  file  INT,"
          "  msize UNSIGNED INT,"
          "  ssize UNSIGNED INT,"
          "  PRIMARY KEY (type, name)"
          ");";

        sqlite3_check(sqlite3_exec(ctx.db, sql_create_tables, 0, 0, 0));
      }

      { // initdb statements
        const char sql_get[] =
          "SELECT file, msize, ssize FROM data WHERE type = ?1 AND name = ?2;";

        sqlite3_check(sqlite3_prepare(ctx.db, sql_get, sizeof(sql_get),
          &ctx.stmt_get, 0));
      }

      { // get the max data file name
        i32 dbr;
        sqlite3_stmt *stmt_max;
        const char    sql_max[] =
          "SELECT IFNULL(MAX(file), 0) AS last_file FROM data";

        sqlite3_check(sqlite3_prepare(ctx.db, sql_max, sizeof(sql_max),
          &stmt_max, 0));

        dbr = sqlite3_step(stmt_max);
        XASSERT(dbr == SQLITE_DONE || dbr == SQLITE_ROW, "query failed.");

        ctx.last_file = (dbr == SQLITE_ROW) ?
          (u32)sqlite3_column_int(stmt_max, 0) : 0;

        sqlite3_check(sqlite3_finalize(stmt_max));
      }
    }

    // opens a resource data file
    // returns NULL if the resource data does not extist
    FILE *open_read(DataManager &ctx, const DataId &id, u32 &size)
    {
      char szData[MAX_PATH];

      sqlite3_check(sqlite3_bind_int(ctx.stmt_get, 1, (i32)id.fields.type));
      sqlite3_check(sqlite3_bind_int(ctx.stmt_get, 2, (i32)id.fields.name));
      
      int dbr = sqlite3_step(ctx.stmt_get);
      XASSERT(dbr == SQLITE_ROW, "Could not find the resource named %d", id.fields.name);

      sprintf(szData, "%s\\%d", ctx.dir, sqlite3_column_int(ctx.stmt_get, 0));
      size = (u32)sqlite3_column_int(ctx.stmt_get, 1);

      sqlite3_check(sqlite3_reset(ctx.stmt_get));
      sqlite3_check(sqlite3_clear_bindings(ctx.stmt_get));

      return fopen(szData, "rb");
    }

    FILE *open_stream(DataManager &ctx, const DataId &id, u32 &size)
    {
      char szData[MAX_PATH];

      sqlite3_check(sqlite3_bind_int(ctx.stmt_get, 1, (i32)id.fields.type));
      sqlite3_check(sqlite3_bind_int(ctx.stmt_get, 2, (i32)id.fields.name));

      sqlite3_check_with(sqlite3_step(ctx.stmt_get), SQLITE_ROW);
      sprintf(szData, "%s\\%d", ctx.dir, sqlite3_column_int(ctx.stmt_get, 0));
      const u32 offset = (u32)sqlite3_column_int(ctx.stmt_get, 1);
      size = (u32)sqlite3_column_int(ctx.stmt_get, 2);

      sqlite3_check(sqlite3_reset(ctx.stmt_get));
      sqlite3_check(sqlite3_clear_bindings(ctx.stmt_get));

      FILE *stream = fopen(szData, "rb");
      fseek(stream, offset, SEEK_SET);
      return stream;
    }

    // creates and opens a data file for the specified resource id
    // if the resource already has an associated data file, a new one is created
    FILE *open_write(DataManager &ctx, const DataId &id)
    {
      char szData[MAX_PATH];

      DataManager::Change q;
      q.type = DataManager::CHANGE_UPDATE;
      q.file = ++ctx.last_file;
      hash::set(ctx.changes, id.as64, q);

      sprintf(szData, "%s\\%d", ctx.dir, q.file);

      return fopen(szData, "wb");
    }

    void close_write(DataManager &ctx, const DataId &id, FILE *file, u32 memory_size, u32 stream_size)
    {
      DataManager::Change q = *hash::get(ctx.changes, id.as64);
      q.msize = memory_size;
      q.ssize = stream_size;
      hash::set(ctx.changes, id.as64, q);
      fflush(file);
      fclose(file);
    }

    // deletes a resource data file
    void remove(DataManager &ctx, const DataId &id)
    {
      DataManager::Change q;
      q.type = DataManager::CHANGE_DELETE;
      hash::set(ctx.changes, id.as64, q);
    }

    // save resource/data file mapping changes in the db
    void save(DataManager &ctx)
    {
      DataId pid;
      char tmp[BUFSIZ];

      const Hash<DataManager::Change>::Entry *c;
      const Hash<DataManager::Change>::Entry *end = hash::end(ctx.changes);

      if (hash::size(ctx.changes) == 0)
        return;

      // delete query
      const char sql_delete[] = "DELETE FROM data WHERE (0 = 1) ";
      const char sql_or[] = " OR (type=%d AND name=%d)";

      i32 sql_count = 0;
      array::clear(ctx.sqlbuf);
      ctx.sqlbuf << sql_delete;

      for (c = hash::begin(ctx.changes); c < end; c++) {
        if (c->value.type == DataManager::CHANGE_DELETE) {
          if (sql_count >= sqlite3_limit(ctx.db, SQLITE_LIMIT_EXPR_DEPTH, -1)
              || array::size(ctx.sqlbuf) >= (u32)sqlite3_limit(ctx.db, SQLITE_LIMIT_SQL_LENGTH, -1) + sizeof(sql_or)+1)
          {
            ctx.sqlbuf << ";";
            sqlite3_check(sqlite3_exec(ctx.db, c_str(ctx.sqlbuf), 0, 0, 0));

            sql_count = 0;
            array::clear(ctx.sqlbuf);
            ctx.sqlbuf << sql_delete;
          }
          pid.as64 = c->key;
          sprintf(tmp, sql_or, pid.fields.type, pid.fields.name);
          push(ctx.sqlbuf, tmp, strlen(tmp));
          ++sql_count;
        }
      }
      if (sql_count) {
        ctx.sqlbuf << ";";
        sqlite3_check(sqlite3_exec(ctx.db, c_str(ctx.sqlbuf), 0, 0, 0));
      }

      // replace queries
      const char sql_replace[] = "REPLACE INTO data(type, name, file, msize, ssize) VALUES ";
      const char sql_value[] = "(%d,%d,%d,%d,%d),";

      sql_count = 0;
      array::clear(ctx.sqlbuf);
      ctx.sqlbuf << sql_replace;

      for (c = hash::begin(ctx.changes); c < end; c++) {
        if (c->value.type == DataManager::CHANGE_UPDATE) {
          if (sql_count >= sqlite3_limit(ctx.db, SQLITE_LIMIT_COMPOUND_SELECT, -1)
              || array::size(ctx.sqlbuf) >= (u32)sqlite3_limit(ctx.db, SQLITE_LIMIT_SQL_LENGTH, -1) + sizeof(sql_value))
          {
            array::resize(ctx.sqlbuf, array::size(ctx.sqlbuf) - 1);
            ctx.sqlbuf << ";";
            sqlite3_check(sqlite3_exec(ctx.db, c_str(ctx.sqlbuf), 0, 0, 0));

            sql_count = 0;
            array::clear(ctx.sqlbuf);
            ctx.sqlbuf << sql_replace;
          }
          pid.as64 = c->key;
          sprintf(tmp, sql_value, pid.fields.type, pid.fields.name, c->value.file, c->value.msize, c->value.ssize);
          push(ctx.sqlbuf, tmp, strlen(tmp));
          ++sql_count;
        }
      }
      if (sql_count) {
        array::resize(ctx.sqlbuf, array::size(ctx.sqlbuf) - 1);
        ctx.sqlbuf << ";";
        sqlite3_check(sqlite3_exec(ctx.db, c_str(ctx.sqlbuf), 0, 0, 0));
      }
    }

    // physically delete unused data file
    void clean(DataManager &ctx)
    {
      const char    sql_get_all[] = "SELECT file FROM data;";
      sqlite3_stmt *stmt_get_all;
      Hash<i32>     valid_file(*ctx.allocator);
      const i32     dumb_value = 0;

      sqlite3_check(sqlite3_prepare(ctx.db, sql_get_all, sizeof(sql_get_all), &stmt_get_all, 0));

      while (sqlite3_step(stmt_get_all) == SQLITE_ROW)
        hash::set(valid_file, (u64)sqlite3_column_int(stmt_get_all, 0), dumb_value);

      sqlite3_check(sqlite3_finalize(stmt_get_all));

      { // scan data folder and deleted unused files
        WIN32_FIND_DATA fdFile;
        HANDLE hFind = NULL;

        char path[MAX_PATH];

        // specify a file mask
        sprintf(path, "%s\\*.*", ctx.dir);

        if ((hFind = FindFirstFile(path, &fdFile)) == INVALID_HANDLE_VALUE)
        {
          std::printf("Path not found: [%s]\n", ctx.dir);
          return;
        }

        do {
          // find first file will always return "."
          // and ".." as the first two directories.
          i32 num_file;
          if (strcmp(fdFile.cFileName, ".") != 0
              && strcmp(fdFile.cFileName, "..") != 0
              && sscanf(fdFile.cFileName, "%d%s", &num_file) == 1)
          {
            sprintf(path, "%s\\%d", ctx.dir, num_file);
            if (!hash::has(valid_file, (u64)num_file))
              DeleteFile(path);
          }
        } while (FindNextFile(hFind, &fdFile));

        FindClose(hFind);
      }
    }

    // clean & save before to shutdown the system
    void shutdown(DataManager &ctx)
    {
      hash::clear(ctx.changes);
      sqlite3_check(sqlite3_finalize(ctx.stmt_get));
    }
  }
}