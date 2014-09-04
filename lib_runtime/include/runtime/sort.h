#pragma once

#include "runtime/types.h"

namespace pge
{
  template<typename T>
  static inline void merge_sort(T *data, T *sort_buf, u32 size, bool (*compare)(const T &a, const T &b))
  {
    T *a2[2], *_a, *b;
    i32 curr, shift;

    a2[0] = (data);
    a2[1] = sort_buf;
    for (curr = 0, shift = 0; (1ul << shift) < size; ++shift)
    {
      _a = a2[curr]; b = a2[1 - curr];
      if (shift == 0) {
        T *p = b, *i, *eb = _a + size;
        for (i = _a; i < eb; i += 2)
        {
          if (i == eb - 1) *p++ = *i;
          else {
            if ((*compare)(*(i + 1), *i))
            {
              *p++ = *(i + 1); *p++ = *i;
            }
            else
            {
              *p++ = *i; *p++ = *(i + 1);
            }
          }
        }
      }
      else 
      {
        u32 i, step = 1ul << shift;
        for (i = 0; i < size; i += step << 1)
        {
          T *p, *j, *k, *ea, *eb;
          if (size < i + step)
          {
            ea = _a + size; eb = _a;
          }
          else {
            ea = _a + i + step;
            eb = _a + (size < i + (step << 1) ? size : i + (step << 1));
          }
          j = _a + i; k = _a + i + step; p = b + i;
          while (j < ea && k < eb)
          {
            if ((*compare)(*k, *j)) *p++ = *k++;
            else *p++ = *j++;
          }
          while (j < ea) *p++ = *j++;
          while (k < eb) *p++ = *k++;
        }
      }
      curr = 1 - curr;
    }
    if (curr == 1)
    {
      T *p = a2[0], *i = a2[1], *eb = (data)+size;
      for (; p < eb; ++i) *p++ = *i;
    }
  }
}
