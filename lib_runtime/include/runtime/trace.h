#pragma once

#include "types.h"

namespace pge
{
  typedef void(*MessageHandler) (const char *msg);
  void set_log_handler(MessageHandler handler);
  void set_output_handler(MessageHandler handler);

  void _log(const char *msg, char *file, unsigned line, ...);
  void _output(const char *msg, ...);
  void _stamp(const char *msg, ...);

#ifndef NDEBUG
#define LOG(msg, ...) \
  MULTI_LINE_MACRO_BEGIN \
  pge::_log(msg, __FILE__, __LINE__, __VA_ARGS__); \
  MULTI_LINE_MACRO_END
#define OUTPUT(msg, ...) \
  MULTI_LINE_MACRO_BEGIN \
  pge::_output(msg, __VA_ARGS__); \
  MULTI_LINE_MACRO_END
#define STAMP(msg, ...) \
  MULTI_LINE_MACRO_BEGIN \
  pge::_stamp(msg, __VA_ARGS__); \
  MULTI_LINE_MACRO_END
#else
#define LOG(test, msg, ...) (void(0))
#define OUTPUT(test, msg, ...) (void(0))
#define STAMP(test, msg, ...) (void(0))
#endif

}