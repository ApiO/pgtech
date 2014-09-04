
#include <memory>

#include <runtime/memory.h>
#include <runtime/image.h>

namespace pge
{
  void flip_region(u8 *data, u32 x, u32 y, u32 width, u32 height, u32 image_width, Allocator &a)
  {
    u32 row_pad  = x * 4 * sizeof(u8),
      row_size = image_width * 4 * sizeof(u8);

    u8 *first_row = data + row_pad + y*row_size,
      *last_row  = first_row + (height - 1)*row_size,
      *temp_row = (u8*)a.allocate(width * 4);

    while (first_row < last_row)
    {
      memcpy(temp_row, first_row, width * 4);
      memcpy(first_row, last_row, width * 4);
      memcpy(last_row, temp_row, width * 4);

      first_row += row_size;
      last_row  -= row_size;
    }

    a.deallocate(temp_row);
  }

  void rgba_to_bgra(u8 *data, const u32 size)
  {
    u8 *p = data;
    const u8 *end = data + size;
    u8 tmp;
    while (p < end) {
      tmp = *p;
      *p = *(p + 2);
      *(p + 2) = tmp;
      p+=4;
    }
  }

}