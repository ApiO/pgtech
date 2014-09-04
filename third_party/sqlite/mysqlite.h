#ifndef MYSQLITE_H
#define MYSQLITE_H

#include "runtime/assert.h"
#include "sqlite3.h"

#define sqlite3_check(f)              \
  MULTI_LINE_MACRO_BEGIN              \
  int dbr = f;                        \
  XASSERT(dbr == SQLITE_OK,           \
    "%s failed with status %d: %s",   \
    #f, dbr, sqlite3_errstr(dbr));    \
  MULTI_LINE_MACRO_END

#define sqlite3_check_with(f, x)      \
  MULTI_LINE_MACRO_BEGIN              \
  int dbr = f;                        \
  XASSERT(dbr == x,                   \
    "%s failed with status %d: %s",   \
    #f, dbr, sqlite3_errstr(dbr));    \
  MULTI_LINE_MACRO_END

#endif // MYSQLITE_H