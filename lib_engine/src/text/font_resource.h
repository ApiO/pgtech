#pragma once

#include <stdio.h>

#include <runtime/memory.h>
#include <runtime/murmur_hash.h>
#include <runtime/hash.h>

#include <data/font.h>

#include <resource/resource_manager.h>

namespace pge
{
  const u32 HASH_SIZE = sizeof(Hash<Char>);

  namespace font_resource
  {
    void  register_type(void);
    void *load(FILE *file, u32 name, u32 size);
    void  unload(void *data, u32 name);
    u32   line_height(const FontResource *res);
    bool  has(const FontResource *res, u64 char_key);
    const Char &get(const FontResource *res, u64 char_key);
    u8   *get_page(const FontResource *res, u32 page, u32 &size);
  }

  namespace font_resource
  {
    inline void register_type(void)
    {
      resource_manager::register_type(RESOURCE_TYPE_FONT,
                                      &font_resource::load,
                                      &resource_manager::default_bring_in,
                                      &resource_manager::default_patch_up,
                                      &resource_manager::default_bring_out,
                                      &font_resource::unload);
    }

    inline void *load(FILE *file, u32 name, u32 size)
    {
      (void)name;
      void *resource = memory_globals::default_allocator().allocate(HASH_SIZE + size);
      fread((u8*)resource + HASH_SIZE, size, 1, file);

      new (resource)Hash<Char>(memory_globals::default_allocator());

      FontResource *res = (FontResource*)((u8*)resource + HASH_SIZE);
      CharResource *chr = (CharResource*)((u8*)resource + HASH_SIZE + res->_chars_offset);
      Hash<Char> &chars = *(Hash<Char>*)resource;

      hash::reserve(chars, res->_num_chars);

      for (u32 i = 0; i < res->_num_chars; i++) {
        hash::set(chars, (u64)chr->id, chr->chr);
        chr++;
      }
      return res;
    }

    inline void unload(void *data, u32 name)
    {
      (void)name;
      Hash<Char> *chars = (Hash<Char>*)((u8*)data - HASH_SIZE);
      chars->~Hash();
      memory_globals::default_allocator().deallocate(chars);
    }

    inline u32 line_height(const FontResource *res)
    {
      return res->_line_height;
    }

    inline bool has(const FontResource *res, u64 char_key)
    {
      const Hash<Char> &h = *(const Hash<Char>*)(((const u8*)res) - HASH_SIZE);
      return hash::has(h, char_key);
    }

    inline const Char &get(const FontResource *res, u64 char_key)
    {
      const Hash<Char> &h = *(const Hash<Char>*)(((const u8*)res) - HASH_SIZE);
      return *hash::get(h, char_key);
    }

    inline u8 *get_page(const FontResource *res, u32 page, u32 &size)
    {
      FontResource::Page *p = ((FontResource::Page*)(((u8*)res) + res->_pages_offset)) + page;
      size = p->size;
      return ((u8*)res) + p->offset;
    }
  }
}