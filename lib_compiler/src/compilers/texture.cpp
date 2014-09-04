#include <GL/SOIL.h>
#include <nvtt/nvtt.h>

#include <runtime/types.h>
#include <runtime/trace.h>
#include <runtime/temp_allocator.h>
#include <runtime/image.h>
#include <runtime/murmur_hash.h>

#include <data/texture.h>

#include "compiler_types.h"
#include "compiler.h"
#include "texture_packer.h"

namespace
{
  using namespace pge;

  struct SourceImage
  {
    i32  width, height;
    i32  channels;
    u8  *data;
  };

  struct SourceRegion
  {
    u32 name;
    u32 image;
    i32 x, y;
    i32 width, height;
    i32 margin[4];
  };


  TextureType get_texture_type(const Json &src)
  {
    const char *str = json::get_string(src, json::root(src), "type", NULL);
    if (!str) return TEXTURE_TYPE_FULL;

    if (strcmp(str, "FULL") == 0)    return TEXTURE_TYPE_FULL;
    if (strcmp(str, "TILESET") == 0) return TEXTURE_TYPE_TILESET;
    if (strcmp(str, "ATLAS") == 0)   return TEXTURE_TYPE_ATLAS;

    return TEXTURE_TYPE_FULL;
  }

  bool get_texture_mipmap_filter(const Json &src, const Json &def_conf, const u64 def_conf_id, nvtt::MipmapFilter &filter)
  {
    const char *str = json::get_string(src, json::root(src), "mipmap", NULL);
    if (!str) str = json::get_string(def_conf, def_conf_id, "mipmap", NULL);

    using namespace nvtt;
    if (strcmp(str, "NONE") == 0) { return false; }
    if (strcmp(str, "BOX") == 0) { filter = MipmapFilter::MipmapFilter_Box; return true; }
    if (strcmp(str, "TRIANGLE") == 0) { filter = MipmapFilter::MipmapFilter_Triangle; return true; }
    if (strcmp(str, "KAISER") == 0) { filter = MipmapFilter::MipmapFilter_Kaiser; return true; }

    return false;
  }

  nvtt::Format get_texture_compression(const Json &src, const Json &def_conf, const u64 def_conf_id)
  {
    const char *str = json::get_string(src, json::root(src), "compression", NULL);
    if (!str) str = json::get_string(def_conf, def_conf_id, "compression", NULL);

    using namespace nvtt;
    if (!str) return Format_DXT5;
    if (strcmp(str, "DXT1") == 0)  return Format_DXT1;
    if (strcmp(str, "DXT1a") == 0) return Format_DXT1a;
    if (strcmp(str, "DXT3") == 0)  return Format_DXT3;
    if (strcmp(str, "DXT5") == 0)  return Format_DXT5;
    if (strcmp(str, "DXT5n") == 0) return Format_DXT5n;

    return Format_DXT5;
  }

  bool get_texture_trim(const Json &src, const Json &def_conf, const u64 def_conf_id)
  {
    if (json::has(src, json::root(src), "trim"))
      return json::get_bool(src, json::root(src), "trim");

    return json::get_bool(def_conf, def_conf_id, "trim");
  }

  bool get_texture_flip(const Json &src, const Json &def_conf, const u64 def_conf_id)
  {
    if (json::has(src, json::root(src), "flip"))
      return json::get_bool(src, json::root(src), "flip");

    return json::get_bool(def_conf, def_conf_id, "flip");
  }

  bool set_texture_spacing(TextureType type, const Json &src, u32 &spacing)
  {
    if (json::has(src, json::root(src), "spacing")) {
      spacing = (u32)json::get_integer(src, json::root(src), "spacing");
      return true;
    }

    switch (type) {
      case TEXTURE_TYPE_FULL:
        spacing = 0u;
        return true;
      case TEXTURE_TYPE_ATLAS:
      case TEXTURE_TYPE_TILESET:
        spacing = 1u;
        return true;
      default:
        LOG("Unhandled ResourceType enum value %d", type);
        return false;
    }
  }

  void trim_region(const SourceImage &image, SourceRegion &region)
  {
    i32 line, col,
      y_min = region.y,
      y_max = region.y + region.height - 1,
      x_min = region.x,
      x_max = region.x + region.width - 1;
    bool   found;

    const u8 *l, *c;

    //search y min
    found = false;
    for (line = y_min; line <= y_max; line++) {
      l = image.data + (line * image.width * 4);
      for (col = x_min; col <= x_max; col++) {
        c = l + (col * 4);
        if (*(c + 3) != 0) {
          y_min = line;
          found = true;
          break;
        }
      }
      if (found) break;
    }

    //search y max
    found = false;
    for (line = y_max; line >= y_min; line--) {
      l = image.data + (line * image.width * 4);
      for (col = x_min; col <= x_max; col++) {
        c = l + (col * 4);
        if (*(c + 3) != 0) {
          y_max = line;
          found = true;
          break;
        }
      }
      if (found) break;
    }

    //search x min
    found = false;
    for (col = x_min; col <= x_max; col++) {
      for (line = y_min; line <= y_max; line++) {
        c = image.data + ((line * image.width) + col) * 4;
        if (*(c + 3) != 0) {
          x_min = col;
          found = true;
          break;
        }
      }
      if (found) break;
    }

    //search x max
    found = false;
    for (col = x_max; col >= x_min; col--) {
      for (line = y_min; line <= y_max; line++) {
        c = image.data + ((line * image.width) + col) * 4;
        if (*(c + 3) != 0) {
          x_max = col;
          found = true;
          break;
        }
      }
      if (found) break;
    }

    region.margin[0] = y_min - region.y;
    region.margin[1] = (region.x + region.width - 1) - x_max;
    region.margin[2] = (region.y + region.height - 1) - y_max;
    region.margin[3] = x_min - region.x;

    region.x = x_min;
    region.y = y_min;

    region.height = y_max - y_min + 1;
    region.width  = x_max - x_min + 1;
  }

  bool is_empty_region(const SourceImage &image, const SourceRegion &region)
  {
    i32 line, col,
      y_min = region.y,
      y_max = region.y + region.height - 1,
      x_min = region.x,
      x_max = region.x + region.width - 1;
    const u8 *c;

    for (col = x_min; col <= x_max; col++) {
      for (line = y_min; line <= y_max; line++) {
        c = image.data + ((line * image.width) + col) * 4;
        if (*(c + 3) != 0) return false;
      }
    }
    return true;
  }

  bool load_image_dependency(SourceImage &img, Work &w, const char *name)
  {
    using namespace string_stream;
    TempAllocator4096 ta;
    Buffer buf(ta);
    if (!compiler::load_dependency(w, RESOURCE_TYPE_DATA, name, buf)) {
      LOG("Could not load the image \"%s\" in \"%s\"", name, w.src);
      return false;
    }

    img.data = SOIL_load_image_from_memory((u8*)c_str(buf), buf._size, &img.width, &img.height, &img.channels, SOIL_LOAD_RGBA);
    if (img.data == NULL) {
      LOG("Could not parse the image \"%s\" in \"%s\"", name, w.src);
      return false;
    }
    return true;
  }

  bool compress_image(const u8 *bgra, FILE *stream, const Json &src, const Json &def_conf, const u64 def_conf_id, const i32 width, const i32 height, u32 &size)
  {
    using namespace nvtt;

    InputOptions inputOptions;
    MipmapFilter filter;
    inputOptions.setMipmapGeneration(get_texture_mipmap_filter(src, def_conf, def_conf_id, filter));
    inputOptions.setMipmapFilter(filter);
    inputOptions.setTextureLayout(TextureType_2D, width, height, 1);
    inputOptions.setMipmapData(bgra, width, height, 1, 0, 0);
    inputOptions.setFormat(InputFormat_BGRA_8UB);
    inputOptions.setAlphaMode(AlphaMode_Transparency);

    CompressionOptions compressionOptions;
    compressionOptions.setFormat(get_texture_compression(src, def_conf, def_conf_id));
    compressionOptions.setQuality(Quality_Fastest);

    OutputOptions outputOptions;
    outputOptions.setFileStream(stream, false);
    outputOptions.setOutputHeader(true);

    const u32 start = ftell(stream);
    Compressor compressor;
    bool compressed = compressor.process(inputOptions, compressionOptions, outputOptions);
    if (!compressed) {
      LOG("Image compression failed.");
      return false;
    }

    size = ftell(stream) - start;
    return true;
  }

  u32 write_page_data(const RectanglePage &page,
                      const Array<SourceRegion> &source_regions,
                      const Array<SourceImage> &source_images,
                      const Array<PackedRectangle> &packed_rectangles,
                      const Json &src, const Json &def_conf, const u64 def_conf_id,
                      FILE *stream, u32 &size, Allocator &a)
  {
    i32 page_size = sizeof(u8)* page.height * page.width * 4;
    u8 *page_img  = (u8*)a.allocate(page_size);

    // init page as full transparent image
    {
      u32 *comp = (u32*)page_img;
      u32 *end  = (u32*)(page_img + page_size);
      for (; comp < end; comp++) { *comp = 0; }
    }

    // render rectangles into the page
    for (u32 i = 0; i < page.num_rectangles; i++) {
      const PackedRectangle &pr  = packed_rectangles[page.first_rectangle + i];
      const SourceRegion    &sr  = source_regions[pr.input];
      const SourceImage     &img = source_images[sr.image];

      const u8 *src_start = img.data + 4 * (sr.x + sr.y * img.width);
      u8 *dest_start = page_img + 4 * (pr.x + pr.y * page.width);

      u32 src_row_size = img.width * 4 * sizeof(u8);
      u32 dest_y, dest_x;

      for (i32 y = 0; y < sr.height; y++) {
        for (i32 x = 0; x < sr.width; x++) {
          const u8 *src_pix = src_start + (y * src_row_size + x * 4 * sizeof(u8));
          // find coresponding pix in page
          if (pr.rotated) {
            dest_x = y;
            dest_y = sr.width - 1 - x;
          } else {
            dest_x = x;
            dest_y = y;
          }

          u8 *dest_pix = dest_start + (dest_x + dest_y*page.width) * 4;
          i32 a_count = 0;
          u8 r = 0, g = 0, b = 0;
          if (*(src_pix + 3) == 0) { // fix white border issue
            for (int yy = y - 1; yy <= y + 1; yy++) {
              for (int xx = x - 1; xx <= x + 1; xx++) {
                if ((xx >= 0) && (yy >= 0) && (xx < sr.width) && (yy < sr.height)) {
                  const u8 *adj_pix = src_start + (yy * src_row_size + xx * 4 * sizeof(u8));
                  if (*(adj_pix + 3) != 0) {
                    r += *(adj_pix + 0);
                    g += *(adj_pix + 1);
                    b += *(adj_pix + 2);
                    a_count++;
                  }
                }
              }
            }
          } 
          // copy pix from source into the page converting rgba to bgra
          if (a_count == 0){
            *dest_pix       = *(src_pix + 2);
            *(dest_pix + 1) = *(src_pix + 1);
            *(dest_pix + 2) = *(src_pix + 0);
            *(dest_pix + 3) = *(src_pix + 3);
          } else {
            *dest_pix       = (u8)(b / a_count);
            *(dest_pix + 1) = (u8)(g / a_count);
            *(dest_pix + 2) = (u8)(r / a_count);
            *(dest_pix + 3) = (u8)0u;
          }

          // copy pix from source into the page converting rgba to bgra & premultiply
          /*const f32 qa = (f32)(*(src_pix + 3)) / 255.0f;
          *dest_pix       =  (u8)(qa * (*(src_pix + 2)));
          *(dest_pix + 1) =  (u8)(qa * (*(src_pix + 1)));
          *(dest_pix + 2) =  (u8)(qa * (*(src_pix + 0)));
          *(dest_pix + 3) = *(src_pix + 3);*/
        }
      }
    }

    /*
    static int cpt = 0;
    rgba_to_bgra(page_img, page.width * page.height * 4);
    char file_name[MAX_PATH];
    sprintf(file_name, "c:/temp/p_%d.bmp", cpt++);
    SOIL_save_image(file_name, SOIL_SAVE_TYPE_BMP, page.width, page.height, 4, page_img);
    */

    //compress & save page buf to bin
    if (!compress_image(page_img, stream, src, def_conf, def_conf_id, page.width, page.height, size)) {
      LOG("Packed tilset compression fails");
    }
    a.deallocate(page_img);
    return size;
  }

  //------------------------------------------------------

  static bool load_full(const Json &src, const Json &def_conf, const u64 def_conf_id, Array<SourceImage> &source_images, Array<SourceRegion> &source_regions, Work &w, Allocator &a)
  {
    const char *img_name = json::get_string(src, json::root(src), "file");
    SourceImage img;
    { // load the image
      using namespace string_stream;
      Buffer buf(a);
      i32 channel;

      if (!compiler::load_dependency(w, RESOURCE_TYPE_DATA, img_name, buf)) return false;
      img.data = SOIL_load_image_from_memory((u8*)c_str(buf), buf._size, &img.width, &img.height, &channel, SOIL_LOAD_RGBA);

      if (img.data == NULL) {
        LOG("Could not load the image \"%s\" in \"%s\"", img_name, w.src);
        return false;
      }
    }

    SourceRegion r;
    { // create a region for the whole image
      memset(&r, 0, sizeof(SourceRegion));
      r.width = img.width;
      r.height = img.height;
    }

    if (is_empty_region(img, r)) {
      LOG("Texture of type full can't be empty.");
      return false;
    }

    if (get_texture_trim(src, def_conf, def_conf_id))
      trim_region(img, r);

    if (get_texture_flip(src, def_conf, def_conf_id))
      flip_region(img.data, r.x, r.y, r.width, r.height, img.width, a);

    array::push_back(source_regions, r);
    array::push_back(source_images, img);

    return true;
  }

  static bool load_tileset(const Json &src, const Json &def_conf, const u64 def_conf_id,
                           Array<SourceImage> &source_images,
                           Array<SourceRegion> &source_regions,
                           Array<u32> &empty_tiles,
                           Work &w, Allocator &a)
  {
    i32 tile_w = 0, tile_h = 0;
    i32 tile_spacing = 0;

    const Json::Node &tile_size = json::get_node(src, json::root(src), "tile_size");
    tile_w = json::get_integer(src, tile_size.id, 0);
    tile_h = json::get_integer(src, tile_size.id, 1);

    if (json::has(src, json::root(src), "tile_spacing")) {
      tile_spacing = json::get_node(src, json::root(src), "tile_spacing").value.integer;
    }

    SourceImage img;
    load_image_dependency(img, w, json::get_string(src, json::root(src), "file"));

    i32 col_count = img.width / tile_w;
    i32 row_count = img.height / tile_h;

    // Setup tile region for packing
    for (i32 row = 0; row < row_count; row++) {
      for (i32 col = 0; col < col_count; col++) {
        SourceRegion r;
        memset(&r, 0, sizeof(SourceRegion));
        r.name = ((u32)(row << 16) | (u16)col);
        r.x = col*(tile_w + tile_spacing);
        r.y = row*(tile_h + tile_spacing);
        r.width =  tile_w;
        r.height = tile_h;

        if (is_empty_region(img, r)) {
          array::push_back(empty_tiles, r.name);
          continue;
        }

        if (get_texture_trim(src, def_conf, def_conf_id))
          trim_region(img, r);

        if (get_texture_flip(src, def_conf, def_conf_id))
          flip_region(img.data, r.x, r.y, r.width, r.height, img.width, a);

        array::push_back(source_regions, r);
      }
    }
    array::push_back(source_images, img);
    return true;
  }

  static bool load_atlas(const Json &src, const Json &def_conf, const u64 def_conf_id,
                         Array<SourceImage> &source_images,
                         Array<SourceRegion> &source_regions,
                         Array<u32> &empty_tiles,
                         Work &w, Allocator &a)
  {
    TempAllocator1024 ta(a);
    Hash<u32> file_index(ta);
    const u64 regions = json::get_id(src, json::root(src), "regions");

    for (i32 i=0; i < json::size(src, regions); i++) {
      const Json::Node &region = json::get_node(src, regions, i);
      const u64 position = json::has(src, region.id, "position") ? json::get_id(src, region.id, "position") : json::NO_NODE;
      const u64 size     = json::has(src, region.id, "size") ? json::get_id(src, region.id, "size") : json::NO_NODE;
      const char *fname  = json::get_string(src, region.id, "file");
      const u32 fid      = murmur_hash_32(fname);

      // init the region
      SourceRegion r;
      memset(&r, 0, sizeof(SourceRegion));
      r.name   = compiler::create_id_string(w, region.name);
      r.image  = hash::get(file_index, fid, array::size(source_images));
      if (position == json::NO_NODE) {
        r.x = r.y = 0;
      } else {
        r.x = json::get_integer(src, position, 0);
        r.y = json::get_integer(src, position, 1);
      }

      // load image if necessary
      if (r.image == array::size(source_images)) {
        SourceImage img;
        load_image_dependency(img, w, fname);
        hash::set(file_index, fid, r.image);
        array::push_back(source_images, img);
      }
      const SourceImage &img = source_images[r.image];

      // set the region size
      if (size == json::NO_NODE) {
        r.width  = img.width;
        r.height = img.height;
      } else {
        r.width  = json::get_integer(src, size, 0);
        r.height = json::get_integer(src, size, 1);
      }

      if (is_empty_region(img, r)) {
        // discard the region if it's empty
        array::push_back(empty_tiles, r.name);
        continue;
      }

      // preporcess the region
      if (get_texture_trim(src, def_conf, def_conf_id))
        trim_region(img, r);

      if (get_texture_flip(src, def_conf, def_conf_id))
        flip_region(img.data, r.x, r.y, r.width, r.height, img.width, a);

      array::push_back(source_regions, r);
    }

    if (array::size(source_regions) == 0) {
      LOG("Atlas : all regions are fully transparent");
      return false;
    }
    return true;
  }

  static void clean(Array<SourceImage> source_images)
  {
    for (u32 i = 0; i < array::size(source_images); i++)
      free(source_images[i].data);
  }
}

namespace pge
{
  TextureCompiler::TextureCompiler(Allocator &a, StringPool &sp)
    : JsonCompiler(a, RESOURCE_TYPE_TEXTURE, sp) {}

  bool TextureCompiler::compile(Work &w)
  {
    if (!load_json(w)) return false;
    const Json &compilers_config = *w.project->compilers_config;
    u64   def_conf_id    = json::get_node(compilers_config, json::root(compilers_config), "texture").id;

    TextureResource res;
    res.type = (u16)get_texture_type(jsn);

    Array<SourceImage>     source_images(*a);
    Array<SourceRegion>    source_regions(*a);
    Array<PackedRectangle> packed_rectangles(*a);
    Array<RectanglePage>   packed_pages(*a);
    Array<u32>             empty_regions(*a);

    bool data_ready = false;
    switch (res.type) {
      case TEXTURE_TYPE_FULL:
        data_ready = load_full(jsn, compilers_config, def_conf_id, source_images, source_regions, w, *a);
        break;
      case TEXTURE_TYPE_TILESET:
        data_ready = load_tileset(jsn, compilers_config, def_conf_id, source_images, source_regions, empty_regions, w, *a);
        break;
      case TEXTURE_TYPE_ATLAS:
        data_ready = load_atlas(jsn, compilers_config, def_conf_id, source_images, source_regions, empty_regions, w, *a);
        break;
      default:
        LOG("Unhandled TextureType enum value %d", res.type);
        return false;
    }

    if (!data_ready) {
      clean(source_images);
      return false;
    }

    if (array::size(source_regions) == 0) {
      LOG("No region found in %s", w.src);
      clean(source_images);
      return false;
    }

    // Pack regions
    {
      TempAllocator2048 ta;
      Array<InputRectangle> source_rects(ta);

      array::resize(source_rects, array::size(source_regions));
      for (u32 i = 0; i < array::size(source_regions); i++) {
        source_rects[i].width  = source_regions[i].width;
        source_rects[i].height = source_regions[i].height;
      }

      const i32  edge_max = json::get_integer(compilers_config, def_conf_id, "edge_max_size");
      const bool rotate   = json::get_bool(compilers_config, def_conf_id, "rotate");
      u32 spacing;
      if (!set_texture_spacing((TextureType)res.type, jsn, spacing)) return false;

      if (!texture_packer::process(source_rects, packed_rectangles, packed_pages, edge_max, rotate, spacing, *a)) {
        clean(source_images);
        return false;
      }
    }

    res.num_regions = (u16)array::size(source_regions);
    res.num_empty_regions = (u16)array::size(empty_regions);
    res.num_pages = (u16)array::size(packed_pages);

    // write texture header
    fwrite(&res, sizeof(TextureResource), 1, w.data);

    // write texture regions
    for (u32 i = 0; i < res.num_regions; i++) {
      const PackedRectangle &pr = packed_rectangles[i];
      const SourceRegion &sr    = source_regions[pr.input];

      TextureResource::Region r;
      r.name = sr.name;
      r.width = sr.width;
      r.height = sr.height;
      r.page = (u16)pr.page;
      r.x = pr.x;
      r.y = pr.y;
      r.rotated = pr.rotated;
      memcpy(r.margin, sr.margin, sizeof(i32)* 4);

      fwrite(&r, sizeof(TextureResource::Region), 1, w.data);
    }

    // write empty regions
    fwrite(array::begin(empty_regions), sizeof(u32), res.num_empty_regions, w.data);

    // skip enough space to write page headers
    const u32 page_headers_start = ftell(w.data);
    const u32 page_data_start    = page_headers_start + sizeof(TextureResource::Page) * res.num_pages;
    fseek(w.data, page_data_start, SEEK_SET);

    { // write page data populating page headers
      TempAllocator1024 ta;
      Array<TextureResource::Page> res_pages(ta);
      array::reserve(res_pages, res.num_pages);
      u32 offset = page_data_start;

      for (u32 i = 0; i < res.num_pages; i++) {
        TextureResource::Page p;
        p.data_offset = offset;
        p.width = packed_pages[i].width;
        p.height = packed_pages[i].height;
        offset += write_page_data(packed_pages[i], source_regions, source_images, packed_rectangles, jsn, compilers_config, def_conf_id, w.data, p.data_size, *a);
        array::push_back(res_pages, p);
      }

      // rewind & write page headers
      fseek(w.data, page_headers_start, SEEK_SET);
      fwrite(array::begin(res_pages), sizeof(TextureResource::Page), res.num_pages, w.data);
    }

    clean(source_images);

    return true;
  }
}