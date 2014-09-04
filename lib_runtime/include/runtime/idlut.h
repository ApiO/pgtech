#pragma once

#include "runtime/assert.h"
#include "runtime/array.h"
#include "runtime/collection_types.h"

namespace pge
{
  /// Id Lookup Table Container inspired from bitsquid
  /// http://www.altdevblogaday.com/2011/09/23/managing-decoupling-part-4-the-id-lookup-table/

  namespace idlut
  {
    /// The number of elements in the tree.
    template<typename T> u32 size(const IdLookupTable<T> &t);

    /// Returns true if the specified id exists in the hash.
    template<typename T> bool has(const IdLookupTable<T> &t, u64 id);

    /// Returns the value stored for the specified id, or deffault if the id.
    /// does not exist in the hash.
    template<typename T> const T &lookup(const IdLookupTable<T> &t, u64 id, const T &deffault);
    template<typename T>       T *lookup(IdLookupTable<T> &t, u64 id);

    /// Add a value into the table and returns its id.
    template<typename T> u64 add(IdLookupTable<T> &t, const T &value);

    /// Removes the id from the tree if it exists.
    template<typename T> void remove(IdLookupTable<T> &t, u64 id);

    /// Resizes the id lookup table to the specified size.
    template<typename T> void reserve(IdLookupTable<T> &t, u32 capacity);

    /// Removes all items in the id lookup table (does not free memory).
    template<typename T> void clear(IdLookupTable<T> &t);

    /// Returns a pointer to the first entry in the id lookup table, can be used to
    /// efficiently iterate over the elements (in random order).
    template<typename T> typename IdLookupTable<T>::Entry *begin(IdLookupTable<T> &t);
    template<typename T> const typename IdLookupTable<T>::Entry *begin(const IdLookupTable<T> &t);
    template<typename T> typename IdLookupTable<T>::Entry *end(IdLookupTable<T> &t);
    template<typename T> const typename IdLookupTable<T>::Entry *end(const IdLookupTable<T> &t);
  }

  namespace idlut_internal
  {
    const u32 INDEX_MASK = 0xffffffffu;
    const u32 INDEX_FREE = 0xffffffffu;
    const u64 ID_ADD     = 0x100000000;

    template<typename T> void grow(IdLookupTable<T> &t, u32 capacity)
    {
      u32 old_size = array::size(t._indices);
      array::resize(t._indices, capacity);
      array::reserve(t._data, capacity);

      for (u32 i = old_size; i < capacity; ++i) {
        t._indices[i].id = i;
        t._indices[i].next = i + 1;
        t._indices[i].index = idlut_internal::INDEX_FREE;
      }

      if (array::size(t._data) == old_size) {
        t._freelist_dequeue = old_size;
        t._freelist_enqueue = capacity - 1;
        return;
      }

      t._indices[capacity - 1].next = t._freelist_dequeue;
      t._freelist_dequeue = old_size;
    }
  }

  namespace idlut
  {
    template<typename T> inline u32 size(const IdLookupTable<T> &t)
    {
      return array::size(t._data);
    }

    template<typename T> inline bool has(const IdLookupTable<T> &t, u64 id)
    {
      if (array::size(t._indices) == 0) return false;
      const typename IdLookupTable<T>::Index &in = t._indices[id & idlut_internal::INDEX_MASK];
      return in.id == id && in.index != idlut_internal::INDEX_FREE;
    }

    template<typename T> inline const T &lookup(const IdLookupTable<T> &t, u64 id, const T &deffault)
    {
      const typename IdLookupTable<T>::Index &in = t._indices[id & idlut_internal::INDEX_MASK];
      return (in.id == id && in.index != idlut_internal::INDEX_FREE) ?
        t._data[in.index].value : deffault;
    }

    template<typename T> inline T *lookup(IdLookupTable<T> &t, u64 id)
    {
      const typename IdLookupTable<T>::Index &in = t._indices[id & idlut_internal::INDEX_MASK];
      return (in.id == id && in.index != idlut_internal::INDEX_FREE) ?
        &t._data[in.index].value : NULL;
    }

    template<typename T> inline const typename IdLookupTable<T>::Entry *begin(const IdLookupTable<T> &t)
    {
      return array::begin(t._data);
    }
    template<typename T> inline typename IdLookupTable<T>::Entry *begin(IdLookupTable<T> &t)
    {
      return array::begin(t._data);
    }

    template<typename T> inline const typename IdLookupTable<T>::Entry *end(const IdLookupTable<T> &t)
    {
      return array::end(t._data);
    }

    template<typename T> inline typename IdLookupTable<T>::Entry *end(IdLookupTable<T> &t)
    {
      return array::end(t._data);
    }

    template<typename T> void reserve(IdLookupTable<T> &t, u32 capacity)
    {
      if (array::size(t._indices) < capacity)
        idlut_internal::grow(t, capacity);
    }

    template<typename T> void clear(IdLookupTable<T> &t)
    {
      array::clear(t._indices);
      array::clear(t._data);

      t._freelist_dequeue = 0;
      t._freelist_enqueue = 0;
    }

    template<typename T> u64 add(IdLookupTable<T> &t, const T &value)
    {
      if (array::size(t._data) >= array::size(t._indices))
        idlut_internal::grow(t, array::size(t._indices) * 2 + 8);

      typename IdLookupTable<T>::Index &in = t._indices[t._freelist_dequeue];

      in.id += idlut_internal::ID_ADD;
      ASSERT(in.index == idlut_internal::INDEX_FREE);
      in.index = array::size(t._data);
      t._freelist_dequeue = in.next;

      typename IdLookupTable<T>::Entry e;
      e.value = value;
      e.id = in.id;
      array::push_back(t._data, e);

      return e.id;
    }

    template<typename T> void remove(IdLookupTable<T> &t, u64 id)
    {
      typename IdLookupTable<T>::Index &in = t._indices[id & idlut_internal::INDEX_MASK];

      // copy the last item at position of the one to delete
      typename IdLookupTable<T>::Entry &e = t._data[in.index];
      e = array::pop_back(t._data);

      // update the indices
      t._indices[e.id & idlut_internal::INDEX_MASK].index = in.index;

      ASSERT(in.index != idlut_internal::INDEX_FREE);
      in.index = idlut_internal::INDEX_FREE;
      t._indices[t._freelist_enqueue].next = id & idlut_internal::INDEX_MASK;
      t._freelist_enqueue = id & idlut_internal::INDEX_MASK;

      if (array::size(t._data) == array::size(t._indices) - 1)
        t._freelist_dequeue = t._freelist_enqueue;
    }
  }

  template <typename T> IdLookupTable<T>::IdLookupTable(Allocator &a) :
    _indices(a), _data(a), _freelist_dequeue(0), _freelist_enqueue(0)
  {}
}