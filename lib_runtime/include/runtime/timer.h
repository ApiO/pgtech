#pragma once 

#ifdef TIMER
  #include "runtime/types.h"
  #ifdef _WIN32   // Windows system specific
    #include <windows.h>
  #else           // Unix based system specific
    #include <sys/time.h>
  #endif
  #include <stdlib.h>
#endif

#ifndef TIMER
  #define Timer(v) (void(0))
  #define start_timer(t) (void(0))
  #define stop_timer(t)  (void(0))
  #define get_elapsed_time(t)(0.f)
  #define get_elapsed_time_in_sec(t)(0.f)
  #define get_elapsed_time_in_ms(t)(0.f)
  #define get_elapsed_time_in_micos(t)(0.f)
#else
  namespace pge
  {
    struct Timer
    {
#ifdef _WIN32
      Timer()
        : start_time_in_micros(0), end_time_in_micros(0), stopped(false)
      {
        QueryPerformanceFrequency(&frequency);
        start_count.QuadPart = 0;
        end_count.QuadPart   = 0;
      }
#else
      _Timer()
        : start_time_in_micros(0), end_time_in_micros(0), stopped(false)
      {
        start_count.tv_sec = start_count.tv_usec = 0;
        end_count.tv_sec   = end_count.tv_usec   = 0;
      }
#endif
      f64  start_time_in_micros;
      f64  end_time_in_micros;
      bool stopped;
#ifdef _WIN32
      LARGE_INTEGER frequency;
      LARGE_INTEGER start_count;
      LARGE_INTEGER end_count;
#else
      timeval start_count;
      timeval end_count;
#endif
    };
    namespace timer
    {
      void _start(Timer &t);
      void _stop(Timer &t);
      f64  _get_elapsed_time(Timer &t);
      f64  _get_elapsed_time_in_sec(Timer &t);
      f64  _get_elapsed_time_in_ms(Timer &t);
      f64  _get_elapsed_time_in_micos(Timer &t);
    }
  }
  #define start_timer(t) pge::timer::_start(t)
  #define stop_timer(t)  pge::timer::_stop(t)
  #define get_elapsed_time(t)           pge::timer::_get_elapsed_time(t)
  #define get_elapsed_time_in_sec(t)    pge::timer::_get_elapsed_time_in_sec(t)
  #define get_elapsed_time_in_ms(t)     pge::timer::_get_elapsed_time_in_ms(t)
  #define get_elapsed_time_in_micos(t)  pge::timer::_get_elapsed_time_in_micos(t)
#endif


#ifdef TIMER
  namespace pge
  {
    namespace timer
    {
      inline void _start(Timer &t)
      {
        t.stopped = false;
#ifdef WIN32
        QueryPerformanceCounter(&t.start_count);
#else
        gettimeofday(&t.start_count, NULL);
#endif
      }

      inline void _stop(Timer &t)
      {
        t.stopped = true;
#ifdef WIN32
        QueryPerformanceCounter(&t.end_count);
#else
        gettimeofday(&t.end_count, NULL);
#endif
      }

      inline f64 _get_elapsed_time(Timer &t)
      {
        return _get_elapsed_time_in_sec(t);
      }

      inline f64 _get_elapsed_time_in_sec(Timer &t)
      {
        return _get_elapsed_time_in_micos(t) * .000001f;
      }

      inline f64 _get_elapsed_time_in_ms(Timer &t)
      {
        return _get_elapsed_time_in_micos(t) * .001f;
      }

      inline f64 _get_elapsed_time_in_micos(Timer &t)
      {
#ifdef WIN32
        if (!t.stopped)
          QueryPerformanceCounter(&t.end_count);

        t.start_time_in_micros = t.start_count.QuadPart * (1000000.0 / t.frequency.QuadPart);
        t.end_time_in_micros   = t.end_count.QuadPart * (1000000.0 / t.frequency.QuadPart);
#else
        if (!t.stopped)
          gettimeofday(&t.end_count, NULL);

        t.start_time_in_micros = (t.start_count.tv_sec * 1000000.0) + t.start_count.tv_usec;
        t.end_time_in_micros   = (t.end_count.tv_sec * 1000000.0) + t.end_count.tv_usec;
#endif
        return t.end_time_in_micros - t.start_time_in_micros;
      }

    }
  }
#endif