#pragma once

#include "runtime/collection_types.h"
#include "runtime/array.h"

#include <string.h>
#include <stdio.h>

namespace pge
{
	/// Functions for operating on an Array<char> as a stream of characters,
	/// useful for string formatting, etc.
	namespace string_stream
	{
		typedef Array<char> Buffer;

		/// Dumps the item to the stream using a default formatting.
		Buffer & operator<<(Buffer &b, char c);
		Buffer & operator<<(Buffer &b, const char *s);
		Buffer & operator<<(Buffer &b, f32 f);
    Buffer & operator<<(Buffer &b, f64 f);
		Buffer & operator<<(Buffer &b, i32 i);
		Buffer & operator<<(Buffer &b, u32 i);
		Buffer & operator<<(Buffer &b, u64 i);

		/// Uses printf to print formatted data to the stream.
		Buffer & printf(Buffer &b, const char *format, ...);

		/// Pushes the raw data to the stream.
		Buffer & push(Buffer &b, const char *data, u32 n);

		/// Pads the stream with spaces until it is aligned at the specified column.
		/// Can be used to column align data. (Assumes each char is 1 space wide,
		/// i.e. does not work with UTF-8 data.)
		Buffer & tab(Buffer &b, u32 column);

		/// Adds the specified number of c to the stream.
		Buffer & repeat(Buffer &b, u32 count, char c);

		/// Returns the stream as a C-string. There will always be a \0 character
		/// at the end of the returned string. You don't have to explicitly add it
		/// to the buffer.
		const char *c_str(Buffer &b);
	}

	namespace string_stream_internal
	{
		using namespace string_stream;

		template <typename T>
		inline Buffer &printf_small(Buffer &b, const char *fmt, const T &t)
		{
			char s[32];
			snprintf(s, 32, fmt, t);
			return (b << s);
		}
	}

	namespace string_stream
	{
		inline Buffer & operator<<(Buffer &b, char c)
		{
			array::push_back(b, c);
			return b;
		}

		inline Buffer & operator<<(Buffer &b, const char *s)
		{
			return push(b, s, strlen(s));
		}

    inline Buffer & operator<<(Buffer &b, f32 f)
		{
			return string_stream_internal::printf_small(b, "%g", f);
		}

		inline Buffer & operator<<(Buffer &b, f64 f)
		{
			return string_stream_internal::printf_small(b, "%g", f);
		}

		inline Buffer & operator<<(Buffer &b, i32 i)
		{
			return string_stream_internal::printf_small(b, "%d", i);
		}

		inline Buffer & operator<<(Buffer &b, u32 i)
		{
			return string_stream_internal::printf_small(b, "%u", i);
		}

		inline Buffer & operator<<(Buffer &b, u64 i)
		{
			return string_stream_internal::printf_small(b, "%01llx", i);
		}

		inline Buffer & push(Buffer &b, const char *data, u32 n)
		{
			unsigned int end = array::size(b);
			array::resize(b, end + n);
			memcpy(array::begin(b) + end, data, n);
			return b;
		}

		inline const char *c_str(Buffer &b)
		{
			// Ensure there is a \0 at the end of the buffer.
			array::push_back(b, '\0');
			array::pop_back(b);
			return array::begin(b);
		}
	}
}