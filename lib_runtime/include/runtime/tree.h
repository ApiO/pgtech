#pragma once

#include "runtime/collection_types.h"
#include "runtime/array.h"
#include "runtime/idlut.h"
#include "runtime/assert.h"

namespace pge
{
  namespace tree
  {
    const  u64 NO_NODE = 0xFFFFFFFFu;
    struct Iterator { u64 child, next, prev, parent; };

    /// Returns the id of the root value.
    template<typename T> u64 root(const Tree<T> &t);

    /// Adds a value under the specified parent and returns its id.
    template<typename T> u64 add(Tree<T> &t, u64 parent, const T &value);

    /// Returns the value stored for the specified id.
    template<typename T> T &get(Tree<T> &t, u64 id);
    template<typename T> const T &get(const Tree<T> &t, u64 id);

    /// Removes the id from the table if it exists.
    template<typename T> void remove(Tree<T> &t, u64 id);

    /// Moves the node with the specified 'id' under the specified parent.
    template<typename T> void move(Tree<T> &t, u64 id, u64 parent);

    /// Returns true if the specified id exists in the tree.
    template<typename T> bool has(const Tree<T> &t, u64 id);

    /// Returns true if a node has at least one child
    template<typename T> bool has_child(const Tree<T> &t, u64 id);

    /// Reserve enough memory for size nodes.
    template<typename T> void reserve(Tree<T> &t, u32 size);

    /// Removes all nodes in the Tree<T> (does not free memory).
    template<typename T> void clear(Tree<T> &t);

    /// Returns the root value and initializes the specified iterator
    template<typename T> const T *enter(const Tree<T> &t, Iterator &it);

    /// Returns the value of the 'next' node and updates the iterator
    template<typename T> const T *step(const Tree<T> &t, Iterator &it, u64 id);

    /// Returns a pointer to the first entry in the tree, can be used to
    /// efficiently iterate over the elements (in random order).
    template<typename T> typename Tree<T>::Entry *begin(Tree<T> &t);
    template<typename T> const typename Tree<T>::Entry *begin(const Tree<T> &t);
    template<typename T> typename Tree<T>::Entry *begin(Tree<T> &t);
    template<typename T> const typename Tree<T>::Entry *end(const Tree<T> &t);

    template<typename T> u32 size(const Tree<T> &t);
  }

  namespace tree_internal
  {
    template <typename T> static void link_node(Tree<T> &t, u64 id, u64 parent)
    {
      if (parent == tree::NO_NODE) return;

      typename Tree<T>::Node *node = idlut::lookup(t._nodes, id);
      typename Tree<T>::Node *parent_node = idlut::lookup(t._nodes, parent);

      if (parent_node->child != tree::NO_NODE) {
        node->next = parent_node->child;
        idlut::lookup(t._nodes, parent_node->child)->prev = id;
      }
      else {
        node->next = tree::NO_NODE;
      }

      parent_node->child = id;
      node->parent = parent;
      node->prev   = parent;
    }

    template <typename T> static void unlink_node(Tree<T> &t, u64 id)
    {
      typename Tree<T>::Node *node = idlut::lookup(t._nodes, id);
      typename Tree<T>::Node *parent_node = idlut::lookup(t._nodes, node->parent);

      if (node->next != tree::NO_NODE)
        idlut::lookup(t._nodes, node->next)->prev = node->prev;

      if (parent_node->child == id)
        parent_node->child = node->next;
      else
        idlut::lookup(t._nodes, node->prev)->next = node->next;
    }

    template <typename T> static void delete_node(Tree<T> &t, u64 id, bool single)
    {
      typename Tree<T>::Node *node, *last_node;

      do {
        node = idlut::lookup(t._nodes, id);
        last_node = idlut::lookup(t._nodes, array::back(t._data).id);

        t._data[node->index] = t._data[last_node->index];
        last_node->index = node->index;
        array::pop_back(t._data);

        if (!single && node->child != tree::NO_NODE)
          delete_node(t, node->child, single);

        const u64 next = node->next;
        idlut::remove(t._nodes, id);
        id = next;
      } while (!single && id != tree::NO_NODE);
    }

    template<typename T> inline T *step(Tree<T> &t, tree::Iterator &it, u64 id)
    {
      typename Tree<T>::Node *node;

      if (id == tree::NO_NODE) return 0;

      node = idlut::lookup(t._nodes, id);
      it.child  = node->child;
      it.next   = node->next;
      it.prev   = node->prev;
      it.parent = node->parent;
      return &t._data[node->index].value;
    }
  }

  namespace tree
  {
    template<typename T> inline u64 root(const Tree<T> &t)
    {
      return t._root;
    }

    template<typename T> inline T *enter(Tree<T> &t, Iterator &it)
    {
      return tree_internal::step(t, it, t._root);
    }

    template<typename T> inline const T *enter(const Tree<T> &t, Iterator &it)
    {
      return tree_internal::step(t, it, t._root);
    }

    template<typename T> T *step(Tree<T> &t, Iterator &it, u64 id)
    {
      return tree_internal::step(t, it, id);
    }

    template<typename T> const T *step(const Tree<T> &t, Iterator &it, u64 id)
    {
      return tree_internal::step(t, it, id);
    }

    template<typename T> inline typename Tree<T>::Entry *begin(Tree<T> &t)
    {
      return array::begin(t._data);
    }

    template<typename T> inline const typename Tree<T>::Entry *begin(const Tree<T> &t)
    {
      return array::begin(t._data);
    }

    template<typename T> inline typename Tree<T>::Entry *end(Tree<T> &t)
    {
      return array::end(t._data);
    }

    template<typename T> inline const typename Tree<T>::Entry *end(const Tree<T> &t)
    {
      return array::end(t._data);
    }

    template<typename T> inline u32 size(const Tree<T> &t)
    {
      return array::size(t._data);
    }

    template<typename T> void reserve(Tree<T> &t, u32 size)
    {
      idlut::reserve(t._nodes, size);
      array::reserve(t._data, size);
    }

    template<typename T> T &get(Tree<T> &t, u64 id)
    {
      return t._data[idlut::lookup(t._nodes, id)->index].value;
    }

    template<typename T> const T &get(const Tree<T> &t, u64 id)
    {
      return &t._data[idlut::lookup(t._nodes, id)->index].value;
    }

    template<typename T> bool has(const Tree<T> &t, u64 id)
    {
      return idlut::has(t._nodes, id);
    }

    template<typename T> bool has_child(const Tree<T> &t, u64 id)
    {
      const typename Tree<T>::Node def ={ NO_NODE, NO_NODE, NO_NODE, NO_NODE };
      return idlut::lookup(t._nodes, id, def).child != NO_NODE;
    }

    template<typename T> u64 add(Tree<T> &t, u64 parent, const T &value)
    {
      typename Tree<T>::Node  new_node;
      typename Tree<T>::Entry new_entry;

      new_node.child = NO_NODE;
      new_node.index = array::size(t._data);

      new_entry.id = idlut::add(t._nodes, new_node);
      new_entry.value = value;

      array::push_back(t._data, new_entry);

      tree_internal::link_node(t, new_entry.id, parent);
      return new_entry.id;
    }

    template<typename T> void remove(Tree<T> &t, u64 id)
    {
      XASSERT(id != t._root, "Cannot remove the root node.");

      typename Tree<T>::Node *node = idlut::lookup(t._nodes, id);

      if (node->child != NO_NODE)
        tree_internal::delete_node(t, node->child, 0);

      tree_internal::unlink_node(t, id);
      tree_internal::delete_node(t, id, 1);
    }

    template<typename T> void move(Tree<T> &t, u64 id, u64 parent)
    {
      typename Tree<T>::Node *node = idlut::lookup(t._nodes, id);
      XASSERT(node->parent != NO_NODE, "Cannot move the root.");
      typename Tree<T>::Node *new_parent_node = idlut::lookup(t._nodes, parent);

      // do nothing if the node is the parent
      if (id == parent) return;

      tree_internal::unlink_node(t, id);

      // relink the node
      if (new_parent_node->child != NO_NODE) {
        node->next = new_parent_node->child;
        idlut::lookup(t._nodes, new_parent_node->child)->prev = id;
      }
      else {
        node->next = NO_NODE;
      }

      new_parent_node->child = id;
      node->parent = parent;
      node->prev   = parent;
    }

    template<typename T> void print(Tree<T> &s, u64 node, char *txt, i32 indent = 0)
    {
      if (txt[0] == -52) strcpy(txt, "\0");
      for (i32 i = 0; i < indent; i++)
        strcat(txt, "  ");

      char tmp[128];

      sprintf(tmp, "%llu", node);
      strcat(txt, tmp);
      strcat(txt, "\n");

      indent++;

      tree::Iterator it;
      tree::step(s, it, node);

      u64 child_id = it.child;
      tree::step(s, it, child_id);

      while (child_id != tree::NO_NODE)
      {
        print(s, child_id, txt, indent);

        child_id = it.next;
        tree::step(s, it, it.next);
      }
    }
  }

  template<typename T>
  inline Tree<T>::Tree(Allocator &a, const T &value) : _nodes(a), _data(a)
  {
    _root = tree::add(*this, tree::NO_NODE, value);
  };
}