#pragma once

#include "runtime/types.h"
#include "runtime/memory_types.h"

namespace pge
{
  /// Base class for memory allocators.
  ///
  /// Note: Regardless of which allocator is used, prefer to allocate memory in larger chunks
  /// instead of in many small allocations. This helps with data locality, fragmentation,
  /// memory usage tracking, etc.
  class Allocator
  {
  public:
    /// Default alignment for memory allocations.
    static const u32 DEFAULT_ALIGN = 4;

    Allocator() {}
    virtual ~Allocator() {}

    /// Allocates the specified amount of memory aligned to the specified alignment.
    virtual void *allocate(u32 size, u32 align = DEFAULT_ALIGN) = 0;

    /// Frees an allocation previously made with allocate().
    virtual void deallocate(void *p) = 0;

    static const u32 SIZE_NOT_TRACKED = 0xffffffffu;

    /// Returns the amount of usable memory allocated at p. p must be a pointer
    /// returned by allocate() that has not yet been deallocated. The value returned
    /// will be at least the size specified to allocate(), but it can be bigger.
    /// (The allocator may round up the allocation to fit into a set of predefined
    /// slot sizes.)
    ///
    /// Not all allocators support tracking the size of individual allocations.
    /// An allocator that doesn't support it will return SIZE_NOT_TRACKED.
    virtual u32 allocated_size(void *p) = 0;

    /// Returns the total amount of memory allocated by this allocator. Note that the 
    /// size returned can be bigger than the size of all individual allocations made,
    /// because the allocator may keep additional structures.
    ///
    /// If the allocator doesn't track memory, this function returns SIZE_NOT_TRACKED.
    virtual u32 total_allocated() = 0;

  private:
    /// Allocators cannot be copied.
    Allocator(const Allocator& other);
    Allocator& operator=(const Allocator& other);
  };

  /// Creates a new object of type T using the allocator a to allocate the memory.
#define MAKE_NEW(a, T, ...)		(new ((a).allocate(sizeof(T), alignof(T))) T(__VA_ARGS__))

  /// Frees an object allocated with MAKE_NEW.
#define MAKE_DELETE(a, T, p)	MULTI_LINE_MACRO_BEGIN if (p) {(p)->~T(); (a).deallocate(p);} MULTI_LINE_MACRO_END

  /// Functions for accessing global memory data.
  namespace memory_globals {
    /// Initializes the global memory allocators. scratch_buffer_size is the size of the
    /// memory buffer used by the scratch allocators.
    void init(u32 scratch_buffer_size = 4*1024*1024);

    /// Returns a default memory allocator that can be used for most allocations.
    ///
    /// You need to call init() for this allocator to be available.
    Allocator &default_allocator();

    /// Returns a "scratch" allocator that can be used for temporary short-lived memory
    /// allocations. The scratch allocator uses a ring buffer of size scratch_buffer_size
    /// to service the allocations.
    ///
    /// If there is not enough memory in the buffer to match requests for scratch
    /// memory, memory from the default_allocator will be returned instaed.
    Allocator &default_scratch_allocator();

    /// Shuts down the global memory allocators created by init().
    void shutdown();
  }

  namespace memory {
    inline void *align_forward(void *p, u32 align);
    inline void *pointer_add(void *p, u32 bytes);
    inline const void *pointer_add(const void *p, u32 bytes);
    inline void *pointer_sub(void *p, u32 bytes);
    inline const void *pointer_sub(const void *p, u32 bytes);
  }

  // ---------------------------------------------------------------
  // Inline function implementations
  // ---------------------------------------------------------------

  // Aligns p to the specified alignment by moving it forward if necessary
  // and returns the result.
  inline void *memory::align_forward(void *p, u32 align) {
    uintptr_t pi = uintptr_t(p);
    const u32 mod = pi % align;
    if (mod)
      pi += (align - mod);
    return (void *)pi;
  }

  /// Returns the result of advancing p by the specified number of bytes
  inline void *memory::pointer_add(void *p, u32 bytes)	{
    return (void*)((char *)p + bytes);
  }

  inline const void *memory::pointer_add(const void *p, u32 bytes)	{
    return (const void*)((const char *)p + bytes);
  }

  /// Returns the result of moving p back by the specified number of bytes
  inline void *memory::pointer_sub(void *p, u32 bytes)	{
    return (void*)((char *)p - bytes);
  }

  inline const void *memory::pointer_sub(const void *p, u32 bytes)	{
    return (const void*)((const char *)p - bytes);
  }
}
