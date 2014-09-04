#include <string.h>

#include <runtime/array.h>
#include <runtime/murmur_hash.h>
#include <runtime/hash.h>
#include "id_string_manager.h"

namespace pge
{
  namespace id_string_manager
  {
    void startup(IdStringManager &mng, sqlite3 *db)
    {
      mng._db = db;

      { // create the string table
        const char sql[] =
          "CREATE TABLE IF NOT EXISTS strings ("
          "  id  UNSIGNED INT,"
          "  str TEXT,"
          "  PRIMARY KEY (id)"
          ");";
        sqlite3_check(sqlite3_exec(mng._db, sql, 0, 0, 0));
      }

      { // init manager lookup statement
        const char sql[] = "SELECT str FROM strings WHERE id = ?1;";
        sqlite3_check(sqlite3_prepare(mng._db, sql, sizeof(sql), &mng._stmt_lookup, 0));
      }

      { // load existing ids to avoid reinserting them in the db
        sqlite3_stmt *stmt;
        u32 id;
        i32 dbr;

        const char sql[] = "SELECT id FROM strings;";
        char *nullstring = NULL;

        sqlite3_check(sqlite3_prepare(mng._db, sql, sizeof(sql), &stmt, 0));
        dbr = sqlite3_step(stmt);
        while (dbr == SQLITE_ROW) {
          id = (u32)sqlite3_column_int(stmt, 0);
          hash::set(mng._id_strings, id, nullstring);
          dbr = sqlite3_step(stmt);
        }  
        XASSERT(dbr == SQLITE_DONE, "error executing query.");
        sqlite3_check(sqlite3_finalize(stmt));
      }
    }

    u32 create(IdStringManager &mng, const char *str)
    {
      const u32 id  = murmur_hash_32(str);
      const u32 len = strlen(str);

      if (hash::has(mng._id_strings, id))
        return id;

      char *s = (char*)mng._allocator->allocate(len + 1);
      memcpy(s, str, len);
      s[len] = '\0';
      hash::set(mng._id_strings, id, s);

      array::push_back(mng._to_save, id);
      return id;
    }

    bool has(IdStringManager &mng, u32 id)
    {
      return hash::has(mng._id_strings, id);
    }

    char *lookup(IdStringManager &mng, u32 id)
    {
      char *s = NULL;
      s = hash::get(mng._id_strings, id, s);

      if (s != NULL)
        return s;

      sqlite3_check(sqlite3_bind_int(mng._stmt_lookup, 1, (i32)id));

      int dbr = sqlite3_step(mng._stmt_lookup);
      XASSERT(dbr == SQLITE_ROW, "Could not find the id string %u", id);

      const char *dbs = (const char *)sqlite3_column_text(mng._stmt_lookup, 0);
      const u32 len = strlen(dbs);
      s = (char*)mng._allocator->allocate(len + 1);
      strncpy(s, dbs, len);
      s[len] = '\0';

      sqlite3_check(sqlite3_reset(mng._stmt_lookup));
      sqlite3_check(sqlite3_clear_bindings(mng._stmt_lookup));

      hash::set(mng._id_strings, id, s);
      return s;
    }

    void save(IdStringManager &mng)
    {
      using namespace string_stream;

      char tmp_str[BUFSIZ];
      const char sql_insert[] = "INSERT INTO strings(id, str) VALUES ";
      const char sql_value[] = "(%d, '%s'),";

      i32 sql_count = 0;
      array::clear(mng._sqlbuf);
      mng._sqlbuf << sql_insert;
      for (u32 i = 0; i < array::size(mng._to_save); i++) {
        if (sql_count >= sqlite3_limit(mng._db, SQLITE_LIMIT_COMPOUND_SELECT, -1)
            || array::size(mng._sqlbuf) >= (u32)sqlite3_limit(mng._db, SQLITE_LIMIT_SQL_LENGTH, -1) + sizeof(sql_value)) {
          array::resize(mng._sqlbuf, array::size(mng._sqlbuf) - 1);
          mng._sqlbuf << ";";
          sqlite3_check(sqlite3_exec(mng._db, c_str(mng._sqlbuf), 0, 0, 0));

          sql_count = 0;
          array::clear(mng._sqlbuf);
          mng._sqlbuf << sql_insert;
        }

        sprintf(tmp_str, sql_value, (i32)mng._to_save[i], *hash::get(mng._id_strings, mng._to_save[i]));
        push(mng._sqlbuf, tmp_str, strlen(tmp_str));

        ++sql_count;
      }
      if (sql_count) {
        array::resize(mng._sqlbuf, array::size(mng._sqlbuf) - 1);
        mng._sqlbuf << ";";
        sqlite3_check(sqlite3_exec(mng._db, c_str(mng._sqlbuf), 0, 0, 0));
      }

      array::clear(mng._to_save);
    }

    void shutdown(IdStringManager &mng)
    {
      const Hash<char*>::Entry *e, *end = hash::end(mng._id_strings);
      for (e = hash::begin(mng._id_strings); e < end; e++) {
        if (e->value)
          mng._allocator->deallocate(e->value);
      }

      hash::clear(mng._id_strings);
      array::clear(mng._to_save);

      sqlite3_check(sqlite3_finalize(mng._stmt_lookup));
    }
  }
}