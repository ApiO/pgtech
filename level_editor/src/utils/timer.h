#pragma once 

#include <runtime/types.h>
#include <windows.h>
#include <stdlib.h>

namespace app
{
  namespace utils
  {
    struct Timer
    {
      Timer()
        : start_time_in_micros(0), end_time_in_micros(0), stopped(false)
      {
        QueryPerformanceFrequency(&frequency);
        start_count.QuadPart = 0;
        end_count.QuadPart = 0;
      }

      pge::f64  start_time_in_micros;
      pge::f64  end_time_in_micros;
      bool stopped;
      LARGE_INTEGER frequency;
      LARGE_INTEGER start_count;
      LARGE_INTEGER end_count;
    };

    namespace timer
    {
      void start(Timer &t);
      void stop(Timer &t);
      pge::f64  get_elapsed_time(Timer &t);
      pge::f64  get_elapsed_time_in_sec(Timer &t);
      pge::f64  get_elapsed_time_in_ms(Timer &t);
      pge::f64  get_elapsed_time_in_micos(Timer &t);
    }
  }

  namespace utils
  {
    namespace timer
    {
      inline void start(Timer &t)
      {
        t.stopped = false;
        QueryPerformanceCounter(&t.start_count);

      }

      inline void stop(Timer &t)
      {
        t.stopped = true;
        QueryPerformanceCounter(&t.end_count);
      }

      inline pge::f64 get_elapsed_time(Timer &t)
      {
        return get_elapsed_time_in_sec(t);
      }

      inline pge::f64 get_elapsed_time_in_sec(Timer &t)
      {
        return get_elapsed_time_in_micos(t) * .000001f;
      }

      inline pge::f64 get_elapsed_time_in_ms(Timer &t)
      {
        return get_elapsed_time_in_micos(t) * .001f;
      }

      inline pge::f64 get_elapsed_time_in_micos(Timer &t)
      {
        if (!t.stopped)
          QueryPerformanceCounter(&t.end_count);

        t.start_time_in_micros = t.start_count.QuadPart * (1000000.0 / t.frequency.QuadPart);
        t.end_time_in_micros = t.end_count.QuadPart * (1000000.0 / t.frequency.QuadPart);

        return t.end_time_in_micros - t.start_time_in_micros;
      }

    }
  }
}