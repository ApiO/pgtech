#pragma once

#include "runtime/types.h"
#include "runtime/memory_types.h"

namespace pge
{
  /// Dynamically resizable array of POD objects.
	template<typename T> struct Array
	{
		Array  (Allocator &a);
		~Array ();

		Array (const Array &other);
		Array &operator=(const Array &other);
		
		T       &operator[](u32 i);
		const T &operator[](u32 i) const;

		Allocator *_allocator;
		u32 _size;
		u32 _capacity;
		T  *_data;
	};

  /// A double-ended queue/ring buffer.
	template <typename T> struct Queue
	{
		Queue(Allocator &a);

		T       &operator[](u32 i);
		const T &operator[](u32 i) const;

		Array<T> _data;
		u32      _size;
		u32      _offset;
	};

	/// Hash from an u64 to POD objects. If you want to use a generic key
	/// object, use a hash function to map that object to an u64.
	template<typename T> struct Hash
	{
		Hash(Allocator &a);

		T       &operator[](u64 key);
		const T &operator[](u64 key) const;
		
		struct Entry {
			u64 key;
			u32 next;
			T   value;
		};

		Array<u32>   _buckets;
		Array<Entry> _data;
	};

  template<typename T> struct IdLookupTable
  {
    IdLookupTable(Allocator &a);

    struct Index {
      u64  id;
      u32  index;
      u32  next;
    };

    struct Entry {
			u64 id;
			T   value;
		};

    Array<Index> _indices;
    Array<Entry> _data;
    u32 _freelist_enqueue;
    u32 _freelist_dequeue;
  };

  template<typename T> struct Tree
  {
    Tree(Allocator &a, const T &value);

    struct Node {
      u64  child;  // first child id
      u64  next;   // next brother id
      u64  prev;   // previous brother id
      u64  parent; // parent id
      u32  index;  // index dans _data de la valeur du node
    };

    struct Entry {
      u64 id;
      T   value;
    };

    IdLookupTable<Node> _nodes;
    Array<Entry> _data;
    u64 _root;
  };

  struct StringPool
  {
    StringPool  (Allocator &a);
    ~StringPool ();

    struct Entry {
      char *str;
      i32   refs;
    };

    Hash<u64>   _pointer_to_key;
    Hash<Entry> _map;
    Allocator  *_allocator;
  };

  template<typename T> struct List
  {
    List(Allocator &a);

    struct Node 
    {
      u32   previous;
      u32   next;
      u32   entry_i;
    };

    struct Entry {
			u32 node_i;
			T   value;
		};

		T       &operator[](u32 index);
		const T &operator[](u32 index) const;

    Array<Node>  _nodes;
    Array<Entry> _entries;
    u32          _begin;
    u32          _end;
  };
}