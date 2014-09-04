#include "runtime/string_stream.h"

#include <stdarg.h>

namespace pge 
{
	namespace string_stream
	{
		Buffer & printf(Buffer &b, const char *format, ...)
		{
			va_list args;
			
			va_start(args, format);
			int n = vsnprintf(NULL, 0, format, args);
			va_end(args);

			u32 end = array::size(b);
			array::resize(b, end + n + 1);
			
			va_start(args, format);
			vsnprintf(array::begin(b) + end, n + 1, format, args);
			va_end(args);
			
			array::resize(b, end + n);

			return b;
		}

		Buffer & tab(Buffer &b, u32 column)
		{
			u32 current_column = 0;
			u32 i = array::size(b) - 1;
			while (i != 0xffffffffu && b[i] != '\n' && b[i] != '\r') {
				++current_column;
				--i;
			}
			if (current_column < column)
				repeat(b, column - current_column, ' ');
			return b;
		}

		Buffer & repeat(Buffer &b, u32 count, char c)
		{
			for (u32 i=0; i<count; ++i)
				array::push_back(b, c);
			return b;
		}
	}
}