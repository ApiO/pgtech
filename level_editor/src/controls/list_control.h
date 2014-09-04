#pragma once

#include <runtime/types.h>
#include <runtime/memory.h>
#include <runtime/array.h>

namespace app
{
  namespace controls
  {
    struct ListItem
    {
      ListItem();
      void *data;
      bool  selected;
      bool  enabled;
    };

    inline ListItem::ListItem() : selected(false), enabled(true){}

    class ListControl
    {
    public:
      ListControl(pge::Allocator &_a);
      void add(ListItem &value);
      void remove(pge::u32 index);
      void clear(void);
      void resize(pge::u32 size);

      void     select(pge::u32 i);
      pge::u32 size();
      ListItem &operator[](pge::u32 i);

      void set_multi_select(bool value);
      void clear_selection(void);

    private:
      pge::Array<ListItem> items;
      pge::i32 current_selection;
      bool     multi_select;
    };

    inline ListControl::ListControl(pge::Allocator &_a) : items(_a), current_selection(-1), multi_select(false){}
  }

  namespace controls
  {
    inline void ListControl::clear_selection(void)
    {
      if (!pge::array::size(items) || current_selection == -1)
        return;

      items[current_selection].selected = false;
      current_selection = -1;
    }

    inline void ListControl::set_multi_select(bool value)
    {
      multi_select = value;
    }

    inline void ListControl::add(ListItem &value)
    {
      if (!multi_select && current_selection == -1)
        current_selection = pge::array::size(items);

      pge::array::push_back(items, value);
    }

    inline void ListControl::remove(pge::u32 index)
    {
      if (pge::array::size(items) > 1)
        items[index] = pge::array::pop_back(items);
      else
        pge::array::clear(items);
    }

    inline void ListControl::clear(void)
    {
      pge::array::clear(items);
      current_selection = -1;
    }

    inline void ListControl::select(pge::u32 i)
    {
      if (current_selection != -1)
        items[current_selection].selected = false;

      items[i].selected = true;
      current_selection = i;
    }

    inline pge::u32 ListControl::size()
    {
      return pge::array::size(items);
    }

    inline void ListControl::resize(pge::u32 size)
    {
      pge::array::resize(items, size);
    }

    inline ListItem & ListControl::operator[](pge::u32 i)
    {
      return items[i];
    }
  }
}