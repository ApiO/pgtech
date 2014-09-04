#pragma once

#include <stdint.h>
#include <float.h>

namespace pge
{
  // -------------------------------------------------------------------------
  // Short type aliases: please use that whenever you can
  // -------------------------------------------------------------------------

  typedef uint8_t  u8;
  typedef  int8_t  i8;
  typedef uint16_t u16;
  typedef  int16_t i16;
  typedef uint32_t u32;
  typedef  int32_t i32;
  typedef uint64_t u64;
  typedef  int64_t i64;

  typedef float    f32;
  typedef double   f64;

  // -------------------------------------------------------------------------
  // Endian swapping functions
  // -------------------------------------------------------------------------

  inline u16 swap_u16(u16 value)
  {
    return ((value & 0x00FF) << 8) 
      | ((value & 0xFF00) >> 8);
  }

  inline u32 swap_u32(u32 value)
  {
    return ((value & 0x000000FF) << 24)
      | ((value & 0x0000FF00) << 8)
      | ((value & 0x00FF0000) >> 8)
      | ((value & 0xFF000000) >> 24);
  }

  inline u64 swap_u64(u64 value)
  {
    return ((value & 0x00000000000000FF) << 56)
      | ((value & 0x000000000000FF00) << 40)
      | ((value & 0x0000000000FF0000) << 24)
      | ((value & 0x00000000FF000000) << 8)
      | ((value & 0x000000FF00000000) >> 8)
      | ((value & 0x0000FF0000000000) >> 24)
      | ((value & 0x00FF000000000000) >> 40)
      | ((value & 0xFF00000000000000) >> 56);
  }

  inline f32 swap_f32(f32 value)
  {
    union { u32 asU32; f32 asF32; } u;
    u.asF32 = value;
    u.asU32 = swap_u32(u.asU32);
    return u.asF32;
  }

  inline f64 swap_f64(f64 value)
  {
    union { u64 asU64; f64 asF64; } u;
    u.asF64 = value;
    u.asU64 = swap_u64(u.asU64);
    return u.asF64;
  }

  // -------------------------------------------------------------------------
  // next power of 2 roundups
  // -------------------------------------------------------------------------

  inline u8 next_pow2_u8(u8 value)
  {
    --value;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    ++value;
    return value;
  }

  inline u16 next_pow2_u16(u16 value)
  {
    --value;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    ++value;
    return value;
  }

  inline u32 next_pow2_u32(u32 value)
  {
    --value;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    ++value;
    return value;
  }

  inline u64 next_pow2_u64(u64 value)
  {
    --value;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    ++value;
    return value;
  }

  inline i32 is_pow2_u32 (u32 value)
  {
    return ((value != 0) && ((value & (~value + 1)) == value));
  }

  inline i32 next_multiple(i32 i, i32 n) { return (i + n-1)/n*n; };
  inline u32 next_multiple(u32 i, u32 n) { return (i + n-1)/n*n; };

  // -------------------------------------------------------------------------
  // floating point numbers utilities
  // -------------------------------------------------------------------------

  // 16-bit float conversion
  // Implementation based on ftp://www.fox-toolkit.org/pub/fasthalffloatconversion.pdf-

  extern u32 _mantissa_table[2048];
  extern u32 _exponent_table[64];
  extern u16 _offset_table  [64];

  extern u32 _base_table    [1024];
  extern u16 _shift_table   [512];

  void types_init();

  inline f32 u16_to_f32(u16 value)
  {
    union { u32 asU32; f32 asF32; } u;
    u.asU32 = _mantissa_table[_offset_table[value>>10]+(value&0x3ff)]
      +_exponent_table[value>>10];
    return u.asF32;
  }

  inline u16 f32_to_u16(f32 value)
  {
    union { u32 asU32; f32 asF32; } u;
    u.asF32 = value;
    return (u16)(_base_table[(u.asU32>>23)&0x1ff]
      +((u.asU32&0x007fffff)>>_shift_table[(u.asU32>>23)&0x1ff]));
  }

  // -------------------------------------------------------------------------
  // shit & stuff
  // -------------------------------------------------------------------------

  template<typename T> static inline typename T Min(const T &a, const T &b) { return ((a) < (b)) ? (a) : (b); }
  template<typename T> static inline typename T Max(const T &a, const T &b) { return ((a) > (b)) ? (a) : (b); }
}

// -------------------------------------------------------------------------
// Compiler utilities
// -------------------------------------------------------------------------

#ifndef alignof
  #define alignof(x) __alignof(x)
#endif

#ifndef snprintf
  #define snprintf _snprintf
#endif

#ifndef __thread
  #define __thread __declspec(thread)
#endif

#define MULTI_LINE_MACRO_BEGIN do {
#define MULTI_LINE_MACRO_END      \
  __pragma(warning(push))         \
  __pragma(warning(disable:4127)) \
  } while(0)                      \
  __pragma(warning(pop))