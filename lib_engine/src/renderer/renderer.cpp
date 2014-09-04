#include <windows.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/SOIL.h>

#include <runtime/string_stream.h>
#include <runtime/memory.h>
#include <runtime/assert.h>
#include <runtime/timer.h>
#include <runtime/trace.h>
#include <runtime/sort.h>
#include <runtime/hash.h>

#include <text/font_resource.h>
#include <resource/texture_resource.h>
#include <resource/shader_resource.h>
#include <utils/ogl_debug.h>
#include <window/window.h>
#include <sprite/sprite_system.h>
#include <text/text_system.h>
#include <geometry/geometric_system.h>
#include <particle/particle_system.h>

#include "renderer.h"

#if CHRONO_STEPS
#include <utils/app_watcher.h>
#endif

#if APP_SETUP_INFOS
#include <utils/ogl_debug.h>
#endif

namespace
{
  using namespace pge;
  using namespace pge::string_stream;
  
  //--------------------------------------------------------------------------
  //    SHADER PROGRAM
  //--------------------------------------------------------------------------

  struct ShaderProgram
  {
    ShaderProgram()
      : id(0u), vs(0u), fs(0u), uniform_pv(0u){}
    u32 id;
    u32 vs, fs;
    u32 uniform_pv;
  };

  static bool check_shader_compilation(u32 shader_index)
  {
    glCompileShader(shader_index);

    i32 params = -1;
    glGetShaderiv(shader_index, GL_COMPILE_STATUS, &params);
    if (GL_TRUE != params)
    {
      const i32 max_length = 2048;
      i32 actual_length = 0;
      char log[max_length];

      glGetShaderInfoLog(shader_index, max_length, &actual_length, log);

      XERROR("ERROR: GL shader index %i did not compile\n%s\n", shader_index, log);
    }
    return true;
  }

  static void shader_program_init(ShaderProgram &program, u32 res_name)
  {
    const ShaderResource *res = (ShaderResource *)resource_manager::get(RESOURCE_TYPE_SHADER, res_name);

    const GLchar *source = NULL;
    i32 size;

    // VERTEX SHADER
    if (shader_resource::has_vertex(res))
    {
      source = (const GLchar*)shader_resource::get_vertex(res);
      size = (i32)shader_resource::get_vertex_size(res);
      program.vs = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(program.vs, 1, &source, &size);
      glCompileShader(program.vs);
      ASSERT(check_shader_compilation(program.vs));
    }

    // FRAGMENT SHADER
    if (shader_resource::has_fragment(res))
    {
      source = (const GLchar*)shader_resource::get_fragment(res);
      size = (i32)shader_resource::get_fragment_size(res);
      program.fs = glCreateShader(GL_FRAGMENT_SHADER);
      glShaderSource(program.fs, 1, &source, &size);
      glCompileShader(program.fs);
      ASSERT(check_shader_compilation(program.fs));
    }

    // SHADER PROGRAM
    program.id = glCreateProgram();
    if (shader_resource::has_vertex(res))
      glAttachShader(program.id, program.vs);
    if (shader_resource::has_fragment(res))
      glAttachShader(program.id, program.fs);
    glLinkProgram(program.id);

    // SETUP DEFAULT PROJECTION VIEW
    program.uniform_pv = glGetUniformLocation(program.id, "proj_view");

#if APP_SETUP_INFOS
    ASSERT(check_linking(program.id));
    ASSERT(program_is_valid(program.id));
    print_all(program.id, program.vs, "vs", program.fs, "fs");
#endif
  }

  static void shader_program_shutdown(ShaderProgram &program)
  {
    if (program.vs)
    {
      glDetachShader(program.id, program.vs);
      glDeleteShader(program.vs);
    }
    if (program.fs)
    {
      glDetachShader(program.id, program.fs);
      glDeleteShader(program.fs);
    }
    glDeleteProgram(program.id);
  }


  //--------------------------------------------------------------------------
  //    RENDER
  //--------------------------------------------------------------------------
  
  struct Viewport
  {
    u32 screen_x;
    u32 screen_y;
    u32 width;
    u32 height;
  };

  struct BatchConfig
  {
    Viewport  viewport;
    glm::mat4 projection_view;
    u32       num_graphics;
  };

  struct GraphicBuffer
  {
    GraphicBuffer(Allocator &a)
      : items(a), data(a), batches(a), last_batch(NULL){}
    Array<Graphic>      items;
    Buffer              data;
    Array<BatchConfig>  batches;
    BatchConfig        *last_batch;
    BatchConfig        *current_batch;
  };

  struct Renderer
  {
    Renderer(Allocator &_a)
      : a(&_a), textures(_a), fonts_pages(_a),
      buf(_a), buf_alt(_a),
      front_buffer(&buf), back_buffer(&buf_alt), 
      sort_buf(_a), render_buffer(_a){}
    Hash<u32>      textures;
    Hash<u32>      fonts_pages;
    GraphicBuffer *front_buffer;
    GraphicBuffer *back_buffer;
    GraphicBuffer  buf;
    GraphicBuffer  buf_alt;
    Allocator     *a;
    RenderInit     cb_render_init;
    RenderBegin    cb_render_begin;
    RenderEnd      cb_render_end;
    RenderShutdown cb_render_shutdown;
    ShaderProgram  sprite_program;
    ShaderProgram  primitive_program;
    ShaderProgram  particle_program;
    Array<Graphic> sort_buf;
    RenderBuffer   render_buffer;
  };

  static Renderer *_renderer = NULL;
  
  const f32 bg_component = .11764705882352941176470588235294f;
  //const f32 bg_component = .243137325490196078431372549019608f;

  static bool compare_sort_key(const Graphic &a, const Graphic &b)
  {
    glm::vec4 _a(0.f, 0.f, a.sort_key.z, 1.f);
    glm::vec4 _b(0.f, 0.f, b.sort_key.z, 1.f);

    _a = _renderer->back_buffer->current_batch->projection_view * _a;
    _b = _renderer->back_buffer->current_batch->projection_view * _b;

    return (_a.z == _b.z)
      ? (a.sort_key.value > b.sort_key.value) : (_a.z > _b.z);
  }
}

namespace pge
{
  //--------------------------------------------------------------------------
  //    Header definitions
  //--------------------------------------------------------------------------

  GraphicRange::GraphicRange(DrawMode m, u16 ni, u16 io)
    : mode(m), num_indices(ni), indice_offset(io), blend(BLEND_MODE_NORMAL){}

  RenderBuffer::RenderBuffer(Allocator &a)
    : vertices(a), tex_coord(a), colors(a), indices(a){}

  namespace renderer
  {
    void init(u32 sprite_program, u32 primitive_program, u32 particle_program, 
              RenderInit render_init, RenderBegin render_begin, RenderEnd render_end, RenderShutdown render_shutdown, Allocator &a)
    {
      _renderer = MAKE_NEW(a, Renderer, a);

      _renderer->a = &a;
      
      _renderer->cb_render_init     = render_init;
      _renderer->cb_render_begin    = render_begin;
      _renderer->cb_render_end      = render_end;
      _renderer->cb_render_shutdown = render_shutdown;

#if APP_SETUP_INFOS
      const char *impresive_bar = "-----------------------------------------";
      OUTPUT("\n%s\n\tRnderer init debug info\n%s", impresive_bar, impresive_bar);
#endif

      window::make_current_context();
      window::set_swap_interval(0);

#if APP_SETUP_INFOS
      window::print_info();
#endif

      GLenum err = glewInit();
      XASSERT(GLEW_OK == err, "ERROR: Glew init error: %s", glewGetErrorString(err));

      if (_renderer->cb_render_init)
        _renderer->cb_render_init();

#if APP_SETUP_INFOS
      print_gl_version_info();
      log_gl_params();
#endif

      shader_program_init(_renderer->sprite_program, sprite_program);
      shader_program_init(_renderer->primitive_program, primitive_program);
      shader_program_init(_renderer->particle_program, particle_program);

      glGenVertexArrays(1, &_renderer->render_buffer.vao);
      glGenBuffers(4, _renderer->render_buffer.vbo.buffers);

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      glDisable(GL_CULL_FACE);
    }

    void shutdown(void)
    {
      const Hash<u32>::Entry *entry = hash::begin(_renderer->textures),
        *eend = hash::end(_renderer->textures);
      for (; entry < eend; entry++)
      {
        glDeleteTextures(1, &entry->value);
      }

      entry = hash::begin(_renderer->fonts_pages);
      eend = hash::end(_renderer->fonts_pages);
      for (; entry < eend; entry++)
      {
        glDeleteTextures(1, &entry->value);
      }

      shader_program_shutdown(_renderer->particle_program);
      shader_program_shutdown(_renderer->primitive_program);
      shader_program_shutdown(_renderer->sprite_program);

      glDeleteBuffers(4, _renderer->render_buffer.vbo.buffers);
      glDeleteVertexArrays(1, &_renderer->render_buffer.vao);

      if (_renderer->cb_render_shutdown)
        _renderer->cb_render_shutdown();
      
      // ALLOCATIONS
      MAKE_DELETE((*_renderer->a), Renderer, _renderer);
    }
    
    void render(void)
    {
#if CHRONO_STEPS
      Timer(frame_timer);
      Timer(tmp_timer);
      start_timer(frame_timer);
#endif

      Graphic *graphic = NULL;
      Graphic *tmp = NULL;
      Graphic *gend = NULL;

      BatchConfig *batch = NULL;
      BatchConfig *batch_end = NULL;


      glClearColor(bg_component, bg_component, bg_component, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      if (_renderer->cb_render_begin){
#if CHRONO_STEPS
        start_timer(tmp_timer);
#endif
        _renderer->cb_render_begin();

#if CHRONO_STEPS
        stop_timer(tmp_timer);
        app_watcher::save_value(WL_GAMEPLAY_RENDER_BEGIN_CB, get_elapsed_time_in_ms(tmp_timer));
#endif
      }


      graphic = _renderer->back_buffer->items._data;

      batch = array::begin(_renderer->back_buffer->batches);
      batch_end = array::end(_renderer->back_buffer->batches);

      for (; batch < batch_end; batch++)
      {
        _renderer->back_buffer->current_batch = batch;

        glViewport(batch->viewport.screen_x, batch->viewport.screen_y, batch->viewport.width, batch->viewport.height);

        // Sort
        array::reserve(_renderer->sort_buf, batch->num_graphics);
        merge_sort(graphic, _renderer->sort_buf._data, batch->num_graphics, compare_sort_key);

        // Drawing each graphical item

        gend = graphic + batch->num_graphics;


        for (; graphic < gend;)
        {
          u32 num_items = 1;
          GraphicType type = graphic->type;

          tmp = graphic + 1;
          while (tmp < gend)
          {
            if (tmp->type != type) break;
            num_items++;
            tmp++;
          }

          switch (graphic->type)
          {
          case GRAPHIC_TYPE_SPRITE:
#if CHRONO_STEPS
            start_timer(tmp_timer);
#endif
            glUseProgram(_renderer->sprite_program.id);
            glUniformMatrix4fv(_renderer->sprite_program.uniform_pv, 1, GL_FALSE, (GLfloat*)&batch->projection_view);
            PRINT_GL_LAST_ERROR();

            // sprite::draw(graphic, num_items, render_buffer);
            sprite_system::draw(graphic, num_items, _renderer->render_buffer);
            PRINT_GL_LAST_ERROR();

#if CHRONO_STEPS
            stop_timer(tmp_timer);
            app_watcher::save_value(WL_SPRITE_RENDER, get_elapsed_time_in_ms(tmp_timer));
            app_watcher::save_value(WL_SPRITE_NUM_ITEMS, num_items);
#endif
            PRINT_GL_LAST_ERROR();
            break;
          case GRAPHIC_TYPE_TEXT:
#if CHRONO_STEPS
            start_timer(tmp_timer);
#endif
            glUseProgram(_renderer->sprite_program.id);
            glUniformMatrix4fv(_renderer->sprite_program.uniform_pv, 1, GL_FALSE, (GLfloat*)&batch->projection_view);
            PRINT_GL_LAST_ERROR();

            text_system::draw(graphic, num_items, _renderer->render_buffer);
            PRINT_GL_LAST_ERROR();

#if CHRONO_STEPS
            stop_timer(tmp_timer);
            app_watcher::save_value(WL_TEXT_RENDER, get_elapsed_time_in_ms(tmp_timer));
            app_watcher::save_value(WL_TEXT_NUM_ITEMS, num_items);
#endif
            break;
          case GRAPHIC_TYPE_PRIMITIVE:
#if CHRONO_STEPS
            start_timer(tmp_timer);
#endif
            glUseProgram(_renderer->primitive_program.id);
            glUniformMatrix4fv(_renderer->primitive_program.uniform_pv, 1, GL_FALSE, (GLfloat*)&batch->projection_view);
            PRINT_GL_LAST_ERROR();

            geometric_system::draw(graphic, num_items, _renderer->render_buffer);
            PRINT_GL_LAST_ERROR();
#if CHRONO_STEPS
            stop_timer(tmp_timer);
            app_watcher::save_value(WL_PRIMITIVE_RENDER, get_elapsed_time_in_ms(tmp_timer));
            app_watcher::save_value(WL_PRIMITIVE_NUM_ITEMS, num_items);
#endif
            break;
          case GRAPHIC_TYPE_PARTICLE:
#if CHRONO_STEPS
            start_timer(tmp_timer);
#endif
            glUseProgram(_renderer->particle_program.id);
            glUniformMatrix4fv(_renderer->particle_program.uniform_pv, 1, GL_FALSE, (GLfloat*)&batch->projection_view);
            glUniform1i(glGetUniformLocation(_renderer->particle_program.id, "diffuse"),   0);
            glUniform1i(glGetUniformLocation(_renderer->particle_program.id, "positions"), 1);
            glUniform1i(glGetUniformLocation(_renderer->particle_program.id, "colors"),    2);

            PRINT_GL_LAST_ERROR();

            particle_system::draw(graphic, num_items, _renderer->render_buffer);
            PRINT_GL_LAST_ERROR();
#if CHRONO_STEPS
            stop_timer(tmp_timer);
            app_watcher::save_value(WL_PARTICLE_RENDER, get_elapsed_time_in_ms(tmp_timer));
            app_watcher::save_value(WL_PARTICLE_NUM_ITEMS, num_items);
#endif
            break;
          default:
            XERROR("Graphic item type (%d) not handled", (i32)graphic->type);
          }
          graphic += num_items;
        }
      }

      if (_renderer->cb_render_end){
#if CHRONO_STEPS
        start_timer(tmp_timer);
#endif
        _renderer->cb_render_end();
#if CHRONO_STEPS
        stop_timer(tmp_timer);
        app_watcher::save_value(WL_GAMEPLAY_RENDER_END_CB, get_elapsed_time_in_ms(tmp_timer));
#endif
      }

#if CHRONO_STEPS
      stop_timer(frame_timer);
      app_watcher::save_value(WL_RENDER_THREAD, get_elapsed_time_in_ms(frame_timer));
#endif
    }

    void swap_buffers(void)
    {
#if CHRONO_STEPS
      Timer(timer);
      start_timer(timer);
#endif

      // swap buffers
      GraphicBuffer *gb = _renderer->front_buffer;
      _renderer->front_buffer = _renderer->back_buffer;
      _renderer->back_buffer = gb;

      // clean front buffer
      gb = _renderer->front_buffer;
      array::resize(gb->items, 0u);
      array::resize(gb->data, 0u);
      array::resize(gb->batches, 0u);
      gb->last_batch = NULL;

      window::swap_buffer();

#if CHRONO_STEPS
      stop_timer(timer);
      app_watcher::save_value(WL_GLFW_SWAP_BUFFER, get_elapsed_time_in_ms(timer));
#endif
    }

    void start_batch(u32 screen_x, u32 screen_y, u32 width, u32 height, const glm::mat4 &m)
    {
      BatchConfig batch;
      batch.viewport.screen_x = screen_x;
      batch.viewport.screen_y = screen_y;
      batch.viewport.width    = width;
      batch.viewport.height   = height;
      batch.projection_view   = m;
      batch.num_graphics      = 0u;

      _renderer->back_buffer->last_batch = &array::push_back(_renderer->front_buffer->batches, batch);
    }

    const u8 *get_data(const Graphic *graphic)
    {
      return (u8*)(array::begin(_renderer->back_buffer->data) + graphic->data_offset);
    }

    u8 *create_graphic(GraphicType type, u32 size, SortKey sort_key)
    {
      _renderer->back_buffer->last_batch->num_graphics++;

      Graphic item;
      item.type        = type;
      item.sort_key    = sort_key;
      item.data_offset = array::size(_renderer->front_buffer->data);
      array::push_back(_renderer->front_buffer->items, item);

      array::resize(_renderer->front_buffer->data, array::size(_renderer->front_buffer->data) + size);

      return (u8*)array::begin(_renderer->front_buffer->data) + item.data_offset;
    }

    u32 get_texture(u32 texture_name, u32 region_name)
    {
      const TextureResource *res_texture = (TextureResource*)resource_manager::get(RESOURCE_TYPE_TEXTURE, texture_name);
      const TextureResource::Region *res_region = texture_resource::region(res_texture, region_name);
      if (!res_region)
        return 0;
      const u64 id = (u64)texture_name | ((u64)res_region->page << 32);

      if (!hash::has(_renderer->textures, id))
      {
        const TextureResource::Page *res_page = texture_resource::page(res_texture, res_region);

        const u32 tmp = SOIL_load_OGL_texture_from_memory(
          (unsigned char *)texture_resource::page_data(res_texture, res_page),
          res_page->data_size,
          SOIL_LOAD_AUTO,
          SOIL_CREATE_NEW_ID,
          SOIL_FLAG_DDS_LOAD_DIRECT);

        XASSERT(tmp, "SOIL loading error : %s", SOIL_last_result());
        hash::set(_renderer->textures, id, tmp);
      }
      return *hash::get(_renderer->textures, id);
    }

    u32 get_font(const FontResource *font, u32 page)
    {
      u64 font_page = ((u64)font << 32 | page);
      if (!hash::has(_renderer->fonts_pages, font_page))
      {
        u32 size;
        const u8 *data = font_resource::get_page(font, page, size);

        u32 tmp = SOIL_load_OGL_texture_from_memory(data, size,
                                                    SOIL_LOAD_AUTO,
                                                    SOIL_CREATE_NEW_ID,
                                                    SOIL_FLAG_DDS_LOAD_DIRECT);
        XASSERT(tmp, "SOIL loading error : %s", SOIL_last_result());
        hash::set(_renderer->fonts_pages, font_page, tmp);
      }
      return hash::get(_renderer->fonts_pages, font_page, 0u);
    }

    u32 get_program_id(GraphicType type) {
      switch(type) {
      case GRAPHIC_TYPE_SPRITE:
      case GRAPHIC_TYPE_TEXT:
        return _renderer->sprite_program.id;
      case GRAPHIC_TYPE_PARTICLE:
        return _renderer->particle_program.id;
      case GRAPHIC_TYPE_PRIMITIVE:
        return _renderer->primitive_program.id;
      default:
        XERROR("Graphic item type (%d) not handled", (i32)type);
      }
    }
  }
}