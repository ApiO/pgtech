#pragma once

#include "runtime/types.h"

namespace pge
{
  typedef void(*LogHandler) (const char *msg);
  void set_aseet_log(LogHandler handler);

  void _assert(char *test, char *file, unsigned line, const char *msg, ...);
  void _error(char *file, unsigned line, const char *msg, ...);
}

#ifndef NDEBUG
  #define XERROR(msg, ...) _error(__FILE__, __LINE__, msg, __VA_ARGS__)
  #define XASSERT(test, msg, ...) \
    MULTI_LINE_MACRO_BEGIN \
  if (!(test)) pge::_assert(#test, __FILE__, __LINE__, msg, __VA_ARGS__); \
    MULTI_LINE_MACRO_END
  #define ASSERT(test) XASSERT(test, 0)
#else
  #define XERROR(msg, ...) (void(0))
  #define XASSERT(test, msg, ...) (void(0))
  #define ASSERT(test) (void(0))
#endif
