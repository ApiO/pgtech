#pragma once

#include "runtime/collection_types.h"
#include "runtime/memory.h"

#include <memory>

namespace pge
{
  namespace array
  {
    /// The number of elements in the array.
    template<typename T> u32 size(const Array<T> &a);
    /// Returns true if there are any elements in the array.
    template<typename T> bool any(const Array<T> &a);
    /// Returns true if the array is empty.
    template<typename T> bool empty(const Array<T> &a);

    /// Used to iterate over the array.
    template<typename T> T* begin(Array<T> &a);
    template<typename T> const T* begin(const Array<T> &a);
    template<typename T> T* end(Array<T> &a);
    template<typename T> const T* end(const Array<T> &a);

    /// Returns the first/last element of the array. Don't use these on an
    /// empty array.
    template<typename T> T& front(Array<T> &a);
    template<typename T> const T& front(const Array<T> &a);
    template<typename T> T& back(Array<T> &a);
    template<typename T> const T& back(const Array<T> &a);

    /// Changes the size of the array (does not reallocate memory unless necessary).
    template <typename T> void resize(Array<T> &a, u32 new_size);
    /// Removes all items in the array (does not free memory).
    template <typename T> void clear(Array<T> &a);
    /// Reallocates the array to the specified capacity.
    template<typename T> void set_capacity(Array<T> &a, u32 new_capacity);
    /// Makes sure that the array has at least the specified capacity.
    /// (If not, the array is grown.)
    template <typename T> void reserve(Array<T> &a, u32 new_capacity);
    /// Grows the array using a geometric progression formula, so that the ammortized
    /// cost of push_back() is O(1). If a min_capacity is specified, the array will
    /// grow to at least that capacity.
    template<typename T> void grow(Array<T> &a, u32 min_capacity = 0);
    /// Trims the array so that its capacity matches its size.
    template <typename T> void trim(Array<T> &a);

    /// Pushes the item to the end of the array.
    template<typename T> T& push_back(Array<T> &a, const T &item);
    /// Pops the last item from the array. The array cannot be empty.
    template<typename T> T& pop_back(Array<T> &a);

    template <typename T> void copy(Array<T> &dest, Array<T> &src);
  }

  namespace array
  {
    template<typename T> inline u32 size(const Array<T> &a)       { return a._size; }
    template<typename T> inline bool any(const Array<T> &a)       { return a._size != 0; }
    template<typename T> inline bool empty(const Array<T> &a)     { return a._size == 0; }

    template<typename T> inline T* begin(Array<T> &a)             { return a._data; }
    template<typename T> inline const T* begin(const Array<T> &a) { return a._data; }
    template<typename T> inline T* end(Array<T> &a)               { return a._data + a._size; }
    template<typename T> inline const T* end(const Array<T> &a)   { return a._data + a._size; }

    template<typename T> inline T& front(Array<T> &a)             { return a._data[0]; }
    template<typename T> inline const T& front(const Array<T> &a) { return a._data[0]; }
    template<typename T> inline T& back(Array<T> &a) 					    { return a._data[a._size - 1]; }
    template<typename T> inline const T& back(const Array<T> &a)  { return a._data[a._size - 1]; }

    template <typename T> inline void clear(Array<T> &a) { resize(a, 0); }
    template <typename T> inline void trim(Array<T> &a)  { set_capacity(a, a._size); }

    template <typename T> void resize(Array<T> &a, u32 new_size)
    {
      if (new_size > a._capacity)
        grow(a, new_size);
      a._size = new_size;
    }

    template <typename T> inline void reserve(Array<T> &a, u32 new_capacity)
    {
      if (new_capacity > a._capacity)
        set_capacity(a, new_capacity);
    }

    template<typename T> void set_capacity(Array<T> &a, u32 new_capacity)
    {
      if (new_capacity == a._capacity)
        return;

      if (new_capacity < a._size)
        resize(a, new_capacity);

      T *new_data = 0;
      if (new_capacity > 0) {
        new_data = (T *)a._allocator->allocate(sizeof(T)*new_capacity, alignof(T));
        memcpy(new_data, a._data, sizeof(T)*a._size);
      }
      a._allocator->deallocate(a._data);
      a._data = new_data;
      a._capacity = new_capacity;
    }

    template<typename T> void grow(Array<T> &a, u32 min_capacity)
    {
      u32 new_capacity = a._capacity * 2 + 8;
      if (new_capacity < min_capacity)
        new_capacity = min_capacity;
      set_capacity(a, new_capacity);
    }

    template<typename T> inline T& push_back(Array<T> &a, const T &item)
    {
      if (a._size + 1 > a._capacity)
        grow(a);
      a._data[a._size++] = item;
      return a._data[a._size - 1];
    }

    template<typename T> inline T& pop_back(Array<T> &a) {
      return a._data[--a._size];
    }

    template <typename T> inline void copy(Array<T> &dest, Array<T> &src)
    {
      resize(dest, src._size);
      memcpy(dest._data, src._data, sizeof(T)*src._size);
    }
  }

  template <typename T>
  inline Array<T>::Array(Allocator &allocator) : _allocator(&allocator), _size(0), _capacity(0), _data(0) {}

  template <typename T>
  inline Array<T>::~Array()
  {
    _allocator->deallocate(_data);
  }

  template <typename T>
  Array<T>::Array(const Array<T> &other) : _allocator(other._allocator), _size(0), _capacity(0), _data(0)
  {
    const u32 n = other._size;
    array::set_capacity(*this, n);
    memcpy(_data, other._data, sizeof(T)*n);
    _size = n;
  }

  template <typename T>
  Array<T> &Array<T>::operator=(const Array<T> &other)
  {
    const u32 n = other._size;
    array::resize(*this, n);
    memcpy(_data, other._data, sizeof(T)*n);
    return *this;
  }

  template <typename T>
  inline T & Array<T>::operator[](u32 i)
  {
    return _data[i];
  }

  template <typename T>
  inline const T & Array<T>::operator[](u32 i) const
  {
    return _data[i];
  }
}