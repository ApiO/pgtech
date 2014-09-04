#include <Windows.h>
#include <stdio.h>
#include <time.h>

#include "runtime/trace.h"

#define MSG_MAX_SZ 2048

namespace pge
{
  static void _default_message_handler(const char *msg)
  {
    OutputDebugString(msg);
    //printf(msg);
  }

  static MessageHandler output_handler = _default_message_handler;
  static MessageHandler log_handler = _default_message_handler;

  void set_output_handler(MessageHandler handler)
  {
    output_handler = handler;
  }

  void set_log_handler(MessageHandler handler)
  {
    log_handler = handler;
  }
  
  void _log(const char *msg, char *file, unsigned line, ...)
  {
    if (!msg || log_handler == NULL) return;

    char buf[MSG_MAX_SZ];
    sprintf(buf, "[LOG] %s:(%d)\r\n\t", file, line);

    va_list args;
    va_start(args, line);
    vsprintf(buf + strlen(buf), msg, args);
    va_end(args);

    strcat(buf, "\n");

    log_handler(buf);
  }

  void _output(const char *msg, ...)
  {
    if (!msg || output_handler == NULL) return;

    char buf[MSG_MAX_SZ] = "\0";

    va_list args;
    va_start(args, msg);
    vsprintf(buf + strlen(buf), msg, args);
    va_end(args);

    strcat(buf, "\n");

    output_handler(buf);
  }

  void _stamp(const char *msg, ...)
  {
    if (!msg || output_handler == NULL) return;

    char buf[MSG_MAX_SZ];

    time_t now = time(0);
    tm     tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "[%y%m%d %X] ", &tstruct);

    va_list args;
    va_start(args, msg);
    vsprintf(buf + strlen(buf), msg, args);
    va_end(args);

    strcat(buf, "\n");

    output_handler(buf);
  }
}