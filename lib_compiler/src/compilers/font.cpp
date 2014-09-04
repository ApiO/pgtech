#include <GL/SOIL.h>
#include <nvtt/nvtt.h>

#include <runtime/types.h>
#include <runtime/temp_allocator.h>
#include <runtime/array.h>
#include <runtime/collection_types.h>
#include <runtime/string_stream.h>
#include <runtime/image.h>
#include <runtime/trace.h>

#include <data/font.h>

#include "compiler_types.h"
#include "compiler.h"


namespace
{
  using namespace pge;
  using namespace pge::string_stream;
  using namespace nvtt;

  struct Page
  {
    u32 size;
    u8 *bgra;
  };

  typedef struct Array<Page> Pages;
  const u32 DDS_HEADER_SIZE = sizeof(unsigned long)* 32;

  inline const char *next_line(const char *source)
  {
    source = strchr(source, '\n');

    if (source != NULL) source++;
    if (source != NULL && *source == '\0')
      source = NULL;
    return source;
  }

  inline void get_line(char *line, const char *source)
  {
    if (strchr(source, '\n'))
    {
      memcpy(line, source, (strchr(source, '\n') + 1 - source));
      *strrchr(line, '\n') = '\0';
    }
    else
      memcpy(line, source, strlen(source));

    if (strrchr(line, '\r')) *strrchr(line, '\r') = '\0';
  }


  inline char *find(const char *str, const char *key, i32 *len)
  {
    const char *end;

    char *r = (char*)(strstr(str, key) + strlen(key));

    if (len == NULL) return r;

    end = strchr(r, ' ');
    if (end == NULL) end = strchr(r, '\n');
    if (end == NULL) end = strchr(r, '\0');

    *len = end - r;

    return r;
  }

  inline void set_compress_options(InputOptions &inputOptions, CompressionOptions &compressionOptions, const u8 *bgra, const i32 width, const i32 height)
  {
    inputOptions.setMipmapGeneration(false, -1);
    inputOptions.setTextureLayout(TextureType_2D, width, height, 1);
    inputOptions.setMipmapData(bgra, width, height, 1, 0, 0);
    inputOptions.setFormat(InputFormat_BGRA_8UB);
    inputOptions.setAlphaMode(AlphaMode_Transparency);

    compressionOptions.setFormat(Format_DXT5);
    compressionOptions.setQuality(Quality_Production);
  }

  bool compress_image(const u8 *bgra, FILE *stream, const i32 width, const i32 height)
  {
    InputOptions inputOptions;
    CompressionOptions compressionOptions;

    set_compress_options(inputOptions, compressionOptions, bgra, width, height);

    OutputOptions outputOptions;
    outputOptions.setFileStream(stream, false);
    outputOptions.setOutputHeader(true);

    if (!Compressor().process(inputOptions, compressionOptions, outputOptions))
    {
      LOG("Image compression failed.");
      return false;
    }

    return true;
  }

  bool set_page(Page &page, Work &w, Buffer &buf, const u32 dir_len, const char *line, const char *path, bool flip)
  {
    i32 dep_file_name_len;
    char *dep_file = find(line, "file=", &dep_file_name_len) + 1;
    dep_file_name_len -= 2;

    char dep_name[MAX_PATH];
    strcpy(dep_name, path + dir_len);
    strncat(dep_name, dep_file, dep_file_name_len);
    compiler::normalize_path(dep_name);

    if (!compiler::load_dependency(w, RESOURCE_TYPE_DATA, dep_name, buf)) return false;

    i32 width, height, cnl;
    page.bgra = SOIL_load_image_from_memory((u8*)c_str(buf), buf._size, &width, &height, &cnl, SOIL_LOAD_RGBA);

    if (flip)
      flip_region(page.bgra, 0, 0, width, height, width, *buf._allocator);

    /*
    char file_name[MAX_PATH];
    sprintf(file_name, "d:/temp/p_%x_%d.bmp", page.bgra, &page);
    SOIL_save_image(file_name, SOIL_SAVE_TYPE_BMP, width, height, 4, page.bgra);
    */

    InputOptions inputOptions;
    CompressionOptions compressionOptions;

    set_compress_options(inputOptions, compressionOptions, page.bgra, width, height);

    //compress
    page.size = Compressor().estimateSize(inputOptions, compressionOptions) + DDS_HEADER_SIZE;

    return true;
  }

  void save_chars(const char *source, char *line, bool flip, i32 fh, i32 fw, FILE *data)
  {
    while (source != NULL)
    {
      get_line(line, source);

      CharResource c;
      c.id            = (i32)atoi(find(line, "id=", 0));
      c.chr.x         = (i16)atoi(find(line, "x=", 0));
      c.chr.y         = (i16)atoi(find(line, "y=", 0));
      c.chr.width     = (i16)atoi(find(line, "width=", 0));
      c.chr.height    = (i16)atoi(find(line, "height=", 0));
      c.chr.offset_x  = (i16)atoi(find(line, "xoffset=", 0));
      c.chr.offset_y  = (i16)atoi(find(line, "yoffset=", 0));
      c.chr.x_advance = (i16)atoi(find(line, "xadvance=", 0));
      c.chr.page      = (u16)atoi(find(line, "page=", 0));

      u32 pad_flip = (u32)(flip ? fh - c.chr.y - c.chr.height : c.chr.y);

      c.chr.tex_coord[0] = (f32)c.chr.x / fw;
      c.chr.tex_coord[1] = (f32)pad_flip / fh;
      c.chr.tex_coord[2] = (f32)c.chr.x / fw;
      c.chr.tex_coord[3] = (f32)(c.chr.height + pad_flip) / fh;
      c.chr.tex_coord[4] = (f32)(c.chr.x + c.chr.width) / fw;
      c.chr.tex_coord[5] = (f32)(c.chr.height + pad_flip) / fh;
      c.chr.tex_coord[6] = (f32)(c.chr.x + c.chr.width) / fw;
      c.chr.tex_coord[7] = (f32)pad_flip / fh;

      fwrite(&c, sizeof(CharResource), 1, data);

      source = next_line(source);
    }
  }
}

namespace pge
{
  FontCompiler::FontCompiler(Allocator &a)
    : BinaryCompiler(a, RESOURCE_TYPE_FONT) {}

  bool FontCompiler::compile(Work &w)
  {
    if (!load_bytes(w)) return false;

    const Json &compilers_config = *w.project->compilers_config;
    u64  def_conf_id = json::get_node(compilers_config, json::root(compilers_config), "texture").id;
    bool flip        = json::get_bool(compilers_config, def_conf_id, "flip");

    TempAllocator<2048> ta(*a);
    Buffer dep_buf(*a);
    Pages  pages(ta);

    char line[BUFSIZ];
    const char *source = string_stream::c_str(buf);
    i32  font_width = 0, font_height = 0;

    FontResource res;

    // info
    source = next_line(source);

    // common
    get_line(line, source);
    res._line_height = atoi(find(line, "lineHeight=", 0));
    res._num_pages   = atoi(find(line, "pages=", 0));
    font_width       = atoi(find(line, "scaleW=", 0));
    font_height      = atoi(find(line, "scaleH=", 0));
    array::reserve(pages, res._num_pages);
    source = next_line(source);

    // page
    {
      char path[MAX_PATH];
      char src_dir_full[MAX_PATH];
      u32 dir_len;

      u32 len = strrchr(w.src, '\\') - w.src + 1;
      strncpy(path, w.src, len);
      path[len] = '\0';

      _fullpath(src_dir_full, w.project->src_dir, MAX_PATH);
      dir_len = strlen(src_dir_full) + 1;

      for (u32 i = 0; i < res._num_pages; i++)
      {
        get_line(line, source);
        Page page;
        set_page(page, w, dep_buf, dir_len, line, path, flip);
        array::push_back(pages, page);
        source = next_line(source);
      }
    }

    // gets num chars
    get_line(line, source);
    res._num_chars = atoi(find(line, "count=", 0));
    source = next_line(source);

    //finalizes & writes FontResource
    res._chars_offset = sizeof(FontResource);
    res._pages_offset = res._chars_offset + res._num_chars * sizeof(CharResource);

    fwrite(&res, sizeof(FontResource), 1, w.data);

    // parse char & live write to file
    save_chars(source, line, flip, font_height, font_width, w.data);

    // print pages : size & offset, determiné à la volée
    u32 dds_offset = res._pages_offset + res._num_pages * sizeof(FontResource::Page);
    for (u32 i = 0; i < res._num_pages; i++)
    {
      Page *page = array::begin(pages) + i;

      fwrite(&page->size, sizeof(u32), 1, w.data);
      fwrite(&dds_offset, sizeof(u32), 1, w.data);
      dds_offset += page->size;
    }

    // compress dss et direct write
    for (u32 i = 0; i < res._num_pages; i++)
    {
      Page *page = array::begin(pages) + i;
      rgba_to_bgra(page->bgra, page->size);
      compress_image(page->bgra, w.data, font_width, font_height);

      free(page->bgra);
    }

    return true;
  }
}