#pragma once

#include "runtime/memory.h"
#include "runtime/array.h"

namespace pge 
{
  /// An allocator that uses preallocated fixed size blocks
  ///
  /// This allocator preallocates a page with n blocks using the backing allocator.
  /// If all blocks of a page are used, a new page is allocated with the same 
  /// number of blocks.
  /// If an allocation does not fit a block, the backing allocator is used

  class PoolAllocator : public Allocator
  {
  public:
    PoolAllocator(u32 block_size, u32 page_size, Allocator &backing = memory_globals::default_scratch_allocator(), u32 block_align = DEFAULT_ALIGN);
    virtual ~PoolAllocator();

    virtual void *allocate(u32 size, u32 align = DEFAULT_ALIGN);
    virtual void deallocate(void *);
    virtual u32 allocated_size(void *);
    virtual u32 total_allocated();
  private:
    void create_page();

    Allocator &_backing;
    Array<void*> _pages, _freelist;
    u32 _block_size, _page_size, _block_align;
  };

  // ---------------------------------------------------------------
  // Inline function implementations
  // ---------------------------------------------------------------

  void PoolAllocator::create_page()
  {
    void *page = _backing.allocate(_block_size * _page_size, _block_align);
    array::push_back(_pages, page);

    array::reserve(_freelist, array::size(_pages) * _page_size);

    u8 *last_block = (u8*)array::back(_pages) + _block_size*(_page_size - 1);
    for (u32 i = 0; i < _page_size; i++)
      array::push_back(_freelist, (void*)(last_block - i*_block_size));
  }


  PoolAllocator::PoolAllocator(u32 block_size, u32 page_size, Allocator &backing, u32 block_align) : 
    _backing(backing), _pages(backing), _freelist(backing), 
    _page_size(page_size), _block_align(block_align)
  {
    _block_size = next_multiple(block_size, block_align);
    create_page();
  }

  PoolAllocator::~PoolAllocator()
  {
    for(u32 i = 0; i < array::size(_pages); i++)
      _backing.deallocate(_pages[i]);
  }

  void *PoolAllocator::allocate(u32 size, u32 align)
  {
    if (array::size(_freelist) == 0) 
      create_page();

    void *b = array::back(_freelist);
    void *p = memory::align_forward(b, align);

    if ((u8*)p - (u8*)b + size > _block_size)
      return _backing.allocate(size, align);

    array::pop_back(_freelist);
    return p;
  }

  void PoolAllocator::deallocate(void *p)
  {
    if (!p)
      return;

    for (u32 i = 0; i < array::size(_pages); i++) {
      // if p is in a page range
      if (p >= _pages[i] && (u8*)p < (u8*)_pages[i] + _page_size*_block_size) {
        // add it back to the freelist (removing the alignment padding)
        array::push_back(_freelist, 
          memory::pointer_sub(p, ((u32)p - (u32)_pages[i]) % _block_size));
        return;
      }
    }
    _backing.deallocate(p);
  }

  u32 PoolAllocator::allocated_size(void *p) {
    for (u32 i = 0; i < array::size(_pages); i++) {
      // if p is in a page range
      if (p >= _pages[i] && (u8*)p < (u8*)_pages[i] + _page_size*_block_size) {
        return _block_size;
      }
    }
    return _backing.allocated_size(p);
  }

  u32 PoolAllocator::total_allocated() {
    return array::size(_pages) * _page_size * _block_size;
  }
}
