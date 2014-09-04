#pragma once

#include <stdio.h>
#include <time.h>
#include <exception>
#include <runtime/assert.h>
#include <runtime/trace.h>

namespace pge
{
  const char *LOG_FILE_NAME = "log.txt";

  namespace internal_chrash_handler
  {
    static void log_handler(const char *type, const char *msg)
    {
      FILE *stream = fopen(LOG_FILE_NAME, "ab");

      time_t t = time(0);
      struct tm * now = localtime(&t);

      char buf[BUFSIZ];
      strftime(buf, sizeof(buf), "\n\n[%Y/%m/%d %X] \0", now);
      strcat(buf, type);
      strcat(buf, "\n");

      fwrite(buf, strlen(buf), 1, stream);

      fwrite(msg, strlen(msg), 1, stream);
      fclose(stream);
    }

    static void log_output(const char *msg)
    {
      log_handler("Output", msg);
    }

    static void log_info(const char *msg)
    {
      log_handler("Log", msg);
    }

    static void log_error(const char *msg)
    {
      log_handler("ERROR", msg);
    }

  }

  namespace crash_handler
  {
    typedef void(*MainFunc)(int argc, char * argv[]);

    inline void handle(int argc, char * argv[], MainFunc func)
    {
      using namespace internal_chrash_handler;

      set_output_handler(log_output);
      set_log_handler(log_info);
      set_aseet_log(log_error);

      try
      {
        func(argc, argv);
      }
      catch (const std::exception& e) {
        char msg[BUFSIZ];
        sprintf(msg, "Exception caught: %s", e.what());
        log_handler("Exception", msg);
      }

    }

  }

}