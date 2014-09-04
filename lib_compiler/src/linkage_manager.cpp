#include "runtime/array.h"
#include "runtime/hash.h"
#include "runtime/temp_allocator.h"

#include "linkage_manager.h"

namespace pge
{
  namespace linkage_manager
  {
    using namespace string_stream;

    void startup(LinkageManager &ctx, sqlite3 *db)
    {
      ctx._db = db;

      { // Create the tables.
        const char sql_create_tables[] =
          "CREATE TABLE IF NOT EXISTS deps ("
          "  type      UNSIGNED INT,"
          "  name      UNSIGNED INT,"
          "  dep_type  UNSIGNED INT,"
          "  dep_name  UNSIGNED INT,"
          "  PRIMARY KEY (type, name, dep_type, dep_name)"
          ");"
          "CREATE TABLE IF NOT EXISTS refs ("
          "  type      UNSIGNED INT,"
          "  name      UNSIGNED INT,"
          "  ref_type  UNSIGNED INT,"
          "  ref_name  UNSIGNED INT,"
          "  PRIMARY KEY (type, name, ref_type, ref_name)"
          ");";

        sqlite3_check(sqlite3_exec(ctx._db, sql_create_tables, 0, 0, 0));
      }

      { // init in memory dependencies & dependents
        sqlite3_stmt *stmt;
        i32 dbr;
        DataId res_id, lnk_id;

        // load dependencies & dependents
        const char sql_dep[] = "SELECT DISTINCT type, name, dep_type, dep_name FROM deps;";
        sqlite3_check(sqlite3_prepare(ctx._db, sql_dep, sizeof(sql_dep), &stmt, 0));

        dbr = sqlite3_step(stmt);
        while (dbr == SQLITE_ROW) {
          res_id.fields.type = sqlite3_column_int(stmt, 0);
          res_id.fields.name = sqlite3_column_int(stmt, 1);
          lnk_id.fields.type = sqlite3_column_int(stmt, 2);
          lnk_id.fields.name = sqlite3_column_int(stmt, 3);

          multi_hash::insert(ctx._dependencies, res_id.as64, lnk_id.as64);
          multi_hash::insert(ctx._dependents, lnk_id.as64, res_id.as64);
          dbr = sqlite3_step(stmt);
        };
        XASSERT(dbr == SQLITE_DONE, "error executing query.");
        sqlite3_check(sqlite3_finalize(stmt));

        // load references
        const char sql_ref[] = "SELECT DISTINCT type, name, ref_type, ref_name FROM refs;";
        sqlite3_check(sqlite3_prepare(ctx._db, sql_ref, sizeof(sql_ref), &stmt, 0));
        dbr = sqlite3_step(stmt);
        while (dbr == SQLITE_ROW) {
          res_id.fields.type = sqlite3_column_int(stmt, 0);
          res_id.fields.name = sqlite3_column_int(stmt, 1);
          lnk_id.fields.type = sqlite3_column_int(stmt, 2);
          lnk_id.fields.name = sqlite3_column_int(stmt, 3);

          multi_hash::insert(ctx._references, res_id.as64, lnk_id.as64);
          dbr = sqlite3_step(stmt);
        };
        XASSERT(dbr == SQLITE_DONE, "error executing query.");
        sqlite3_check(sqlite3_finalize(stmt));
      }
    }

    static void get_recursive(const Hash<u64> &data, Array<u64> &result)
    {
      TempAllocator2048 ta;
      const u8 dumb_value = 0;
      Hash<u8> tmp(ta);

      for (u32 i = 0; i < array::size(result); i++) {
        const Hash<u64>::Entry *e = multi_hash::find_first(data, result[i]);
        while (e) {
          if (!hash::has(tmp, e->key)) {
            hash::set(tmp, e->key, dumb_value);
            array::push_back(result, e->key);
          }
          e = multi_hash::find_next(data, e);
        }
      }
    }

    void get_dependencies(const LinkageManager &ctx, u64 res_id, Array<u64> &dependencies, bool recursive)
    {
      multi_hash::get(ctx._dependencies, res_id, dependencies);
      if (recursive)
        get_recursive(ctx._dependencies, dependencies);
    }

    void get_dependents(const LinkageManager &ctx, u64 res_id, Array<u64> &dependents, bool recursive)
    {
      multi_hash::get(ctx._dependents, res_id, dependents);
      if (recursive)
        get_recursive(ctx._dependents, dependents);
    }

    void get_references(const LinkageManager &ctx, u64 res_id, Array<u64> &references, bool recursive)
    {
      multi_hash::get(ctx._references, res_id, references);
      if (recursive)
        get_recursive(ctx._references, references);
    }

    void add_dependency(LinkageManager &ctx, u64 res_id, u64 dep_id)
    {
      // update the in memory representation of dependents & dependencies
      multi_hash::insert(ctx._dependencies, res_id, dep_id);
      multi_hash::insert(ctx._dependents, dep_id, res_id);

      // tag the resource as changed for the DB
      hash::set(ctx._changes, res_id, LinkageManager::CHANGE_UPDATE);
    }

    void add_reference(LinkageManager &ctx, u64 res_id, u64 ref_id)
    {
      // check that the reference is not already defined
      const Hash<u64>::Entry *e = multi_hash::find_first(ctx._references, res_id);
      while (e) {
        if (e->value == ref_id)
          return;
        e = multi_hash::find_next(ctx._references, e);
      }

      // update the in memory representation of references
      multi_hash::insert(ctx._references, res_id, ref_id);

      // tag the resource as changed for the DB
      hash::set(ctx._changes, res_id, LinkageManager::CHANGE_UPDATE);
    }

    void remove_all(LinkageManager &ctx, u64 res_id)
    {
      // remove from dependents
      const Hash<u64>::Entry *d = multi_hash::find_first(ctx._dependencies, res_id);
      while (d) {
        const Hash<u64>::Entry *r = multi_hash::find_first(ctx._dependents, d->key);
        while (r) {
          if (r->value == res_id) multi_hash::remove(ctx._dependents, r);
          r = multi_hash::find_next(ctx._dependents, r);
        }
        d = multi_hash::find_next(ctx._dependencies, d);
      }

      // remove from dependencies
      multi_hash::remove_all(ctx._dependencies, res_id);

      // remove from references
      multi_hash::remove_all(ctx._references, res_id);

      // tag the resource as deleted
      hash::set(ctx._changes, res_id, LinkageManager::CHANGE_DELETE);
    }

    static void sql_remove_from(const char *table, LinkageManager &ctx)
    {
      DataId res_id;
      char   sql_delete[BUFSIZ];
      char   tmp_str[BUFSIZ];

      const char sql_or[] = " OR (type=%d AND name=%d)";

      const Hash<LinkageManager::Change>::Entry *c;
      const Hash<LinkageManager::Change>::Entry *end = hash::end(ctx._changes);

      sprintf(sql_delete, "DELETE FROM %s WHERE (0 = 1) ", table);

      i32 sql_count = 0;
      array::clear(ctx._sqlbuf);
      ctx._sqlbuf << sql_delete;

      for (c = hash::begin(ctx._changes); c < end; c++) {
        if (sql_count >= sqlite3_limit(ctx._db, SQLITE_LIMIT_EXPR_DEPTH, -1)
            || array::size(ctx._sqlbuf) >= (u32)sqlite3_limit(ctx._db, SQLITE_LIMIT_SQL_LENGTH, -1) + sizeof(sql_or)+1) {
          ctx._sqlbuf << ";";
          sqlite3_check(sqlite3_exec(ctx._db, c_str(ctx._sqlbuf), 0, 0, 0));

          sql_count = 0;
          array::clear(ctx._sqlbuf);
          ctx._sqlbuf << sql_delete;
        }
        res_id.as64 = c->key;
        sprintf(tmp_str, sql_or, res_id.fields.type, res_id.fields.name);
        push(ctx._sqlbuf, tmp_str, strlen(tmp_str));
        ++sql_count;
      }

      if (sql_count) {
        ctx._sqlbuf << ";";
        sqlite3_check(sqlite3_exec(ctx._db, c_str(ctx._sqlbuf), 0, 0, 0));
      }
    }

    void sql_insert_into(const char *table, LinkageManager &ctx, Hash<u64> source, const char *lnk_prefix)
    {
      DataId res_id;
      char   sql_insert[BUFSIZ];
      char   tmp_str[BUFSIZ];

      const char sql_value[] = "(%d, %d, %d, %d),";

      const Hash<LinkageManager::Change>::Entry *c;
      const Hash<LinkageManager::Change>::Entry *end = hash::end(ctx._changes);

      sprintf(sql_insert, "INSERT INTO %s(type, name, %s_type, %s_name) VALUES ", table, lnk_prefix, lnk_prefix);

      i32 sql_count = 0;
      array::clear(ctx._sqlbuf);
      ctx._sqlbuf << sql_insert;

      for (c = hash::begin(ctx._changes); c < end; c++) {
        if (c->value == LinkageManager::CHANGE_UPDATE) {
          res_id.as64 = c->key;

          const Hash<u64>::Entry *d = multi_hash::find_first(source, res_id.as64);
          DataId lnk_id;
          while (d) {
            if (sql_count >= sqlite3_limit(ctx._db, SQLITE_LIMIT_COMPOUND_SELECT, -1)
                || array::size(ctx._sqlbuf) >= (u32)sqlite3_limit(ctx._db, SQLITE_LIMIT_SQL_LENGTH, -1) + sizeof(sql_value)) {
              array::resize(ctx._sqlbuf, array::size(ctx._sqlbuf) - 1);
              ctx._sqlbuf << ";";
              sqlite3_check(sqlite3_exec(ctx._db, c_str(ctx._sqlbuf), 0, 0, 0));

              sql_count = 0;
              array::clear(ctx._sqlbuf);
              ctx._sqlbuf << sql_insert;
            }
            lnk_id.as64 = d->value;
            sprintf(tmp_str, sql_value, res_id.fields.type, res_id.fields.name, lnk_id.fields.type, lnk_id.fields.name);
            push(ctx._sqlbuf, tmp_str, strlen(tmp_str));

            d = multi_hash::find_next(source, d);
            ++sql_count;
          }
        }
      }
      if (sql_count) {
        array::resize(ctx._sqlbuf, array::size(ctx._sqlbuf) - 1);
        ctx._sqlbuf << ";";
        sqlite3_check(sqlite3_exec(ctx._db, c_str(ctx._sqlbuf), 0, 0, 0));
      }
    }

    void save(LinkageManager &ctx)
    {
      if (hash::size(ctx._changes) == 0)
        return;

      // remove all changed resource entries
      sql_remove_from("deps", ctx);
      sql_remove_from("refs", ctx);

      // (re)insert the updated resource entries
      sql_insert_into("deps", ctx, ctx._dependencies, "dep");
      sql_insert_into("refs", ctx, ctx._references, "ref");
    }

    void shutdown(LinkageManager &ctx)
    {
      hash::clear(ctx._dependencies);
      hash::clear(ctx._dependents);
      hash::clear(ctx._references);
    }
  }
}