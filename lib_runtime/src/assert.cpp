#include "runtime/assert.h"

#include <Windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG_MAX_SZ 2048

namespace pge
{
  static LogHandler log_handler = NULL;

  void set_aseet_log(LogHandler handler)
  {
    log_handler = handler;
  }

  inline void output(char *buf)
  {
    fprintf(stderr, buf);
    OutputDebugString(buf);
    if (log_handler) log_handler(buf);
  }
    
  void _assert(char *test, char *file, unsigned line, const char *msg, ...)
  {
    char buf[MSG_MAX_SZ];
    sprintf(buf, "\n[ASSERTION FAILED]\n\nMessage:\t");

    if (msg)
    {
      va_list args;
      va_start(args, msg);
      vsprintf(buf + strlen(buf), msg, args);
      va_end(args);
    }

    sprintf(buf + strlen(buf), "\nFailed test: %s\nSource:\t%s(:%d)\n", test, file, line);

    output(buf);

    abort();
  }

  void _error(char *file, unsigned line, const char *msg, ...)
  {
    char buf[MSG_MAX_SZ];
    sprintf(buf, "\n[ERROR]\n\nMessage:\t");

    if (msg)
    {
      va_list args;
      va_start(args, msg);
      vsprintf(buf + strlen(buf), msg, args);
      va_end(args);
    }

    sprintf(buf + strlen(buf), "\n\nSource:\t%s:(%d)\n", file, line);

    output(buf);

    abort();
  }
}