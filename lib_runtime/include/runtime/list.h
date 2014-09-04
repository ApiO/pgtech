#pragma once

#include "runtime/types.h"
#include "runtime/assert.h"
#include "runtime/array.h"
#include "runtime/collection_types.h"

#include "runtime/trace.h"

namespace pge
{
  namespace list
  {
    template<typename T> const typename List<T>::Entry &begin(const List<T> &l);
    template<typename T> const typename List<T>::Entry &end(const List<T> &l);
    template<typename T>       typename List<T>::Entry *begin(List<T> &l);
    template<typename T>       typename List<T>::Entry *end(List<T> &l);

    template<typename T> void insert(List<T> &l, const T value, u32 index);
    template<typename T> void push_front(List<T> &l, const T value);
    template<typename T> u32  push_back(List<T> &l, const T value);

    template<typename T> void remove(List<T> &l, u32 index);
    template<typename T> void pop_front(List<T> &l);
    template<typename T> void pop_back(List<T> &l);

    template<typename T> u32  size(List<T> &l);
    template<typename T> void reserve(List<T> &l, u32 capacity);
    template<typename T> void swap(List<T> &l, u32 i1, u32 i2);
    template<typename T> void clear(List<T> &l);
    template<typename T> void copy(List<T> &dest, List<T> &src);
    template<typename T> u32  get_index(List<T> &l, const u32 entry_i);
    //template<typename T> void check_list(List<T> &l);
  }

  namespace list_internal
  {
    const u32 NO_INDEX = 3452816845u;

    template<typename T> u32 get_node_index(List<T> &l, u32 list_index)
    {
      if (list_index == 0)                  return l._begin;
      if (list_index == l._entries._size - 1) return l._end;

      u32 half_size = l._entries._size / 2;

      if (list_index <= half_size) {
        const typename List<T>::Node *node = &l._nodes[l._nodes[l._begin].next];
        for (u32 i = 1; i <= half_size; i++) {
          if (i == list_index) return l._entries[node->entry_i].node_i;
          node = &l._nodes[node->next];
        }
      }
      else {
        const typename List<T>::Node *node = &l._nodes[l._nodes[l._end].previous];
        for (u32 i = l._entries._size - 2; i > half_size; i--) {
          if (i == list_index) return l._entries[node->entry_i].node_i;
          node = &l._nodes[node->previous];
        }
      }
      return NULL;
    }

    template<typename T> void grow(List<T> &l, u32 capacity)
    {
      u32 size = array::size(l._nodes),
        last_entry_i = size == 0 ? NO_INDEX : l._end;

      array::resize(l._nodes, capacity);

      for (u32 i = size; i < capacity; i++)
      {
        l._nodes[i].previous = last_entry_i;
        l._nodes[i].next     = i + 1;
        l._nodes[i].entry_i = NO_INDEX;
        last_entry_i = i;
      }

      array::reserve(l._entries, capacity);

      //list::check_list(l);
    }
  }

  namespace list
  {
    using namespace list_internal;

    template<typename T> const typename List<T>::Entry &begin(const List<T> &l)
    {
      return l._entries[0];
    }

    template<typename T> const typename List<T>::Entry &end(const List<T> &l)
    {
      return l._entries[l._entries._size];
    }

    template<typename T> typename List<T>::Entry *begin(List<T> &l)
    {
      return &l._entries[0];
    }

    template<typename T> typename List<T>::Entry *end(List<T> &l)
    {
      return &l._entries[l._entries._size];
    }

    template<typename T> void insert(List<T> &l, const T value, u32 index)
    {
      //list::check_list(l);

      XASSERT(index <= l._entries._size,
              "list.insert(): Can't insert item at %d, list size: %d", index, l._entries._size);

      if (array::size(l._nodes) == 0) grow(l, 2);
      if (l._entries._size >= array::size(l._nodes))
        grow(l, array::size(l._nodes) << 1);

      // get new node
      u32 node_i = l._entries._size == 0 ? l._begin : l._nodes[l._end].next;
      //ASSERT(node_i != NO_INDEX);
      typename List<T>::Node &node = l._nodes[node_i];
      node.entry_i = l._entries._size;

      // insert data
      typename List<T>::Entry entry;
      entry.node_i = node_i;
      entry.value = value;
      array::push_back(l._entries, entry);

      // update nodes links

      // first element
      if (l._entries._size == 1)
      {
        l._begin = node_i;

        //list::check_list(l);
        return;
      }
      //push front
      if (index == 0)
      {
        l._nodes[l._begin].previous  = node_i;
        l._nodes[node.previous].next = node.next;
        l._nodes[node.next].previous = node.previous;
        node.next      = l._begin;
        node.previous  = NO_INDEX;
        l._begin = node_i;

        //list::check_list(l);
        return;
      }
      //push back
      if (index == l._entries._size - 1)
      {
        l._nodes[l._end].next = node_i;
        node.previous = l._end;
        l._end = node_i;

        //list::check_list(l);
        return;
      }

      // unlink new node
      l._nodes[node.previous].next = node.next;
      if (node.next < l._nodes._size - 1)
        l._nodes[node.next].previous = node.previous;

      // insert node at specific position
      u32 target_node_i = get_node_index(l, index);
      typename List<T>::Node &target_node = l._nodes[target_node_i];

      l._nodes[target_node.previous].next = node_i;
      node.previous = target_node.previous;
      node.next     = target_node_i;
      target_node.previous = node_i;
      
      //list::check_list(l);
    }

    template<typename T> void push_front(List<T> &l, const T value)
    {
      insert(l, value, 0);
    }

    template<typename T> u32 push_back(List<T> &l, const T value)
    {
      u32 index = l._entries._size;
      insert(l, value, index);
      return index;
    }

    /*
    template<typename T> void check_list(List<T> &l)
    {
      u32 i;

      if (l._entries._size > 0)
        ASSERT(l._nodes[l._end].next != NO_INDEX);      

      //check node linking      
      {
        typename List<T>::Node *node = NULL;
        u32 node_i = l._begin;

        Allocator &a = memory_globals::default_allocator();
        Hash<u32> idces(a);

        for (i = 0; i < l._entries._size; i++)
        {
          node = &l._nodes[node_i];

          if (i < l._entries._size - 1)
          {
            XASSERT(node->next != NO_INDEX,
                    "List check error: Node pointing to null node. Node's index : %u ", i);
          }

          if (hash::has(idces, (u64)node->next))
          {
            const u32 ni = hash::get(idces, (u64)node->next, node_i);
            XASSERT(!hash::has(idces, (u64)node->next),
                    "Node index %u already point to node %u. Current node %u", ni, node->next, i);
          }

          hash::set(idces, (u64)node->next, i);

          XASSERT(node->entry_i < l._entries._size,
                  "List check error: Node's entry index is oustide the list capacity");

          node_i = node->next;
        }
      }

      //check entries' node index unicity      
      for (i = 0; i < l._entries._size; i++)
      {
        u32 node_i = (l._entries._data + i)->node_i;
        for (u32 j = i + 1; j < l._entries._size; j++)
        {
          XASSERT(node_i != (l._entries._data + j)->node_i,
                  "List check error: same node_i used in different entries. node_i = %u found in l._entries indices: %u and %u",
                  node_i, i, j);
        }
      }

      //check node's entry index unicity
      for (i = 0; i < l._entries._size; i++)
      {
        u32 entry_i = get_node_index(l, i);
        for (u32 j = i + 1; j < l._entries._size; j++)
        {
          XASSERT(entry_i != get_node_index(l, j),
                  "List check error: Entry index in node is used twice. entry_i = %u for list index %u and %u",
                  entry_i, i, j);
        }
      }

    }
    //*/

    template<typename T> void remove(List<T> &l, u32 index)
    {
      //list::check_list(l);

      XASSERT(index <= l._entries._size,
              "list.remove(): Can't remove item at %d, list size: %d", index, l._entries._size);

      u32 node_i = get_node_index(l, index);
      typename List<T>::Node &node = l._nodes[node_i];

      // copy the last entry at position of the one to delete to avoid holes, and update node owner entry_i if necessary
      if (node.entry_i < l._entries._size - 1)
      {
        u32 moved_node_i = l._entries[l._entries._size - 1].node_i;
        // moves data
        l._entries[node.entry_i] = l._entries[l._entries._size - 1];
        //updates entry_i in node
        l._nodes[moved_node_i].entry_i = node.entry_i;
      }
      array::resize(l._entries, l._entries._size - 1);
      node.entry_i = NO_INDEX;

      // pop back: set new end
      if (node_i == l._end)
      {
        l._end = node.previous;
        //list::check_list(l);
        return;
      }

      // pop front: set new begin
      if (node_i == l._begin)
      {
        l._nodes[node.next].previous = NO_INDEX;
        l._begin = node.next;
      }
      else
      {
        l._nodes[node.previous].next = node.next;
        l._nodes[node.next].previous = node.previous;
      }

      // move node at the end
      if (l._entries._size > 0)
      {
        typename List<T>::Node &end = l._nodes[l._end];

        node.next     = end.next;
        node.previous = l._end;

        end.next  = node_i;
        if (node.next < l._entries._size) l._nodes[node.next].previous = node_i;
      }
      //list::check_list(l);
    }

    template<typename T> void pop_front(List<T> &l)
    {
      remove(l, 0);
    }

    template<typename T> void pop_back(List<T> &l)
    {
      remove(l, l._entries._size - 1);
    }


    template<typename T> u32 size(List<T> &l)
    {
      return l._entries._size;
    }

    template<typename T> void clear(List<T> &l)
    {
      l._end  = l._begin;
      //array::clear(l._nodes);
      //l._nodes._size = l._nodes._capacity;
      array::clear(l._entries);
    }

    template<typename T> void reserve(List<T> &l, u32 capacity)
    {
      if (array::size(l._nodes) >= capacity) return;
      grow(l, next_pow2_u32(capacity));
    }

    template<typename T> void swap(List<T> &l, u32 i1, u32 i2)
    {
      u32 node_i1 = get_node_index(l, i1),
        node_i2 = get_node_index(l, i2);

      T swap = l._entries[l._nodes[node_i1].entry_i].value;
      l._entries[l._nodes[node_i1].entry_i].value = l._entries[l._nodes[node_i2].entry_i].value;
      l._entries[l._nodes[node_i2].entry_i].value = swap;
    }

    template<typename T> u32  get_index(List<T> &l, const u32 entry_i)
    {
      XASSERT(entry_i < l._entries._size, "Data index is to damn high.");

      u32 index;
      u32 node_i = l._entries[entry_i].node_i;

      if (l._begin == node_i) return 0;
      if (l._end == node_i) return l._entries._size - 1;

      const typename List<T>::Node *node = &l._nodes[l._begin];

      for (index = 1; index < l._entries._size - 1; index++)
      {
        node = &l._nodes[node->next];
        if (node->entry_i == entry_i) return index;
      }

      XERROR("Impossibruuu, list's item's index must have been found.");
#ifdef _DEBUG
      return 0;
#endif
    }
  }

  template <typename T> List<T>::List(Allocator &a) :
    _nodes(a), _entries(a), _begin(0), _end(0) {}

  template <typename T> T & List<T>::operator[](u32 index)
  {
    XASSERT(index <= _entries._size, "Index out of range");
    const u32 node_i = list_internal::get_node_index(*this, index);
    return _entries[_nodes[node_i].entry_i].value;
  }

  template <typename T> const T & List<T>::operator[](u32 index) const
  {
    XASSERT(index <= _entries._size, "Index out of range");
    const u32 node_i = list_internal::get_node_index(*(const_cast<List<T>*>(this)), index);
    return _entries[_nodes[node_i].entry_i].value;
  }

}