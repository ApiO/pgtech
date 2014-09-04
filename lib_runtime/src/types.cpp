#include "runtime/types.h"

// -------------------------------------------------------------------------
// 16-bit float conversion tables intitialization
// -------------------------------------------------------------------------
// Implementation based on ftp://www.fox-toolkit.org/pub/fasthalffloatconversion.pdf
// -------------------------------------------------------------------------

namespace pge
{
  u32 _pge_mantissa_table[2048];
  u32 _pge_exponent_table[64];
  u16 _pge_offset_table  [64];
  u32 _pge_base_table    [1024];
  u16 _pge_shift_table   [512];

  static u32 convert_mantissa(u32 i){
    u32 m=i<<13;           // Zero pad mantissa bits
    u32 e=0;               // Zero exponent
    while(!(m&0x00800000)) // While not normalized
    {
      e-=0x00800000;       // Decrement exponent (1<<23)
      m<<=1;               // Shift mantissa
    }
    m&=~0x00800000;        // Clear leading 1 bit
    e+=0x38800000;         // Adjust bias ((127-14)<<23)
    return m | e;          // Return combined number
  }

  void pge_types_init()
  {
    u32 i; i32 e;

    // u16 to f32 tables initialization
    _pge_mantissa_table[0] = 0;
    for(i = 1;    i < 1023; i++) _pge_mantissa_table[i] = convert_mantissa(i);
    for(i = 1024; i < 2047; i++) _pge_mantissa_table[i] = 0x38000000 + ((i-1024)<<13);

    _pge_exponent_table[0]  = 0;
    _pge_exponent_table[32] = 0x80000000;
    for(i = 1;  i < 30; i++) _pge_exponent_table[i] = i<<23;
    for(i = 33; i < 62; i++) _pge_exponent_table[i] = 0x80000000 + ((i-32)<<23);
    _pge_exponent_table[31] = 0x47800000;
    _pge_exponent_table[63] = 0xC7800000;

    for(i = 0; i < 64; i++) _pge_offset_table[i] = 1024;
    _pge_offset_table[0]  = 0;
    _pge_offset_table[32] = 0;

    // f32 to u16 tables initialization
    for(i=0; i<256; ++i)
    {
      e=i-127;
      if(e<-24)
      { // Very small numbers map to zero
        _pge_base_table [i|0x000]=0x0000;
        _pge_base_table [i|0x100]=0x8000;
        _pge_shift_table[i|0x000]=24;
        _pge_shift_table[i|0x100]=24;
      }
      else if(e<-14)
      { // Small numbers map to denorms
        _pge_base_table [i|0x000]=(0x0400>>(-e-14));
        _pge_base_table [i|0x100]=(0x0400>>(-e-14)) | 0x8000;
        _pge_shift_table[i|0x000]=(u16)-e-1;
        _pge_shift_table[i|0x100]=(u16)-e-1;
      }
      else if(e<=15)
      { // Normal numbers just lose precision
        _pge_base_table [i|0x000]=((e+15)<<10);
        _pge_base_table [i|0x100]=((e+15)<<10) | 0x8000;
        _pge_shift_table[i|0x000]=13;
        _pge_shift_table[i|0x100]=13;
      }
      else if(e<128)
      { // Large numbers map to Infinity
        _pge_base_table [i|0x000]=0x7C00;
        _pge_base_table [i|0x100]=0xFC00;
        _pge_shift_table[i|0x000]=24;
        _pge_shift_table[i|0x100]=24;
      }
      else
      { // Infinity and NaN's stay Infinity and NaN's
        _pge_base_table [i|0x000]=0x7C00;
        _pge_base_table [i|0x100]=0xFC00;
        _pge_shift_table[i|0x000]=13;
        _pge_shift_table[i|0x100]=13;
      }
    }
  }
}