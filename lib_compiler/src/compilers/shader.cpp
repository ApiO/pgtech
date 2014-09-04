#include <Windows.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GLFW/glfw3.h>

#include <runtime/types.h>
#include <runtime/trace.h>
#include <runtime/assert.h>
#include <runtime/string_stream.h>

#include <data/shader.h>

#include "compiler_types.h"
#include "compiler.h"

namespace
{
  using namespace pge;
  using namespace pge::string_stream;

#define GL_MESSAGE_SIZE 2048


  static void glfw_error_callback(int error, const char *description)
  {
    LOG("GLFW error (%d): %s", error, description);
  }

  GLFWwindow *create_window(void)
  {
    GLFWwindow *win = NULL;

    glfwSetErrorCallback(glfw_error_callback);

    int r = glfwInit();
    XASSERT(r, "ERROR: GLFW init error.");

    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    win = glfwCreateWindow(1, 1, "", NULL, NULL);
    XASSERT(win, "ERROR: GLFW window creation.");
    glfwMakeContextCurrent(win);

    GLenum err = glewInit();
    XASSERT(GLEW_OK == err, "ERROR: Glew init error.n", glewGetErrorString(err));

    return win;
  }

  bool program_is_valid(u32 program_index)
  {
    glValidateProgram(program_index);

    int params = -1;
    glGetProgramiv(program_index, GL_VALIDATE_STATUS, &params);

    if (GL_TRUE != params)
    {
      int actual_length = 0;
      char log[GL_MESSAGE_SIZE];

      glGetProgramInfoLog(program_index, GL_MESSAGE_SIZE, &actual_length, log);

      LOG("Program info log for GL index %i:\n%s\n",
          program_index, log);

      return false;
    }

    return true;
  }

  bool check_linking(u32 program_index)
  {
    int params = -1;

    glGetProgramiv(program_index, GL_LINK_STATUS, &params);

    if (GL_TRUE != params)
    {
      i32 actual_length = 0;
      char log[GL_MESSAGE_SIZE];

      glGetProgramInfoLog(program_index, GL_MESSAGE_SIZE, &actual_length, log);

      LOG("ERROR: could not link shader program GL index %i\n%s\n",
          program_index, strlen(log) ? log : "none");

      return false;
    }
    return true;
  }

  bool check_shader_compilation(u32 shader_index)
  {
    glCompileShader(shader_index);

    i32 params = -1;
    glGetShaderiv(shader_index, GL_COMPILE_STATUS, &params);
    if (GL_TRUE != params)
    {
      i32 actual_length = 0;
      char log[GL_MESSAGE_SIZE];

      glGetShaderInfoLog(shader_index, GL_MESSAGE_SIZE, &actual_length, log);

      LOG("ERROR: GL shader index %i did not compile\n%s\n", shader_index, log);

      return false;
    }
    return true;
  }

  bool _compile(Work w, Json &jsn, u32 &vert_index, u32 &frag_index, u32 &program_index, Allocator &a)
  {
    Buffer vert_buf(a), frag_buf(a);
    ShaderResource res;
    const GLchar *source = NULL;
    i32 size;

    // VERTEX SHADER
    if (json::has(jsn, json::root(jsn), "vertex"))
    {
      vert_index = glCreateShader(GL_VERTEX_SHADER);
      if (!compiler::load_dependency(w, RESOURCE_TYPE_DATA,
        json::get_string(jsn, json::root(jsn), "vertex"), vert_buf))
        return false;

      source = (const GLchar*)c_str(vert_buf);
      size   = (i32)vert_buf._size;
      glShaderSource(vert_index, 1, &source, &size);

      glCompileShader(vert_index);
      if (!check_shader_compilation(vert_index)) return false;

      res.vertex_shader_offset = sizeof(ShaderResource);
      res.vertex_shader_size = (u32)size;
    }

    // FRAGMENT SHADER
    if (json::has(jsn, json::root(jsn), "fragment"))
    {
      frag_index = glCreateShader(GL_FRAGMENT_SHADER);
      if (!compiler::load_dependency(w, RESOURCE_TYPE_DATA,
        json::get_string(jsn, json::root(jsn), "fragment"), frag_buf))
        return false;

      source = (const GLchar*)c_str(frag_buf);
      size   = (i32)frag_buf._size;
      glShaderSource(frag_index, 1, &source, &size);

      glCompileShader(frag_index);
      if (!check_shader_compilation(frag_index)) return false;

      res.fragment_shader_size = (u32)size;
    }

    // PROGRAM VALIDATION
    program_index = glCreateProgram();

    glAttachShader(program_index, vert_index);
    glAttachShader(program_index, frag_index);
    glLinkProgram(program_index);

    if (!check_linking(program_index)) return false;

    if (!program_is_valid(program_index)) return false;

    // Writes to bin
    fwrite(&res, sizeof(ShaderResource), 1, w.data);

    if (res.vertex_shader_size)
      fwrite(c_str(vert_buf), sizeof(char), vert_buf._size, w.data);
    
    if (res.fragment_shader_size)
      fwrite(c_str(frag_buf), sizeof(char), frag_buf._size, w.data);
    
    return true;
  }
}

namespace pge
{
  ShaderCompiler::ShaderCompiler(Allocator &a, StringPool &sp)
    : JsonCompiler(a, RESOURCE_TYPE_SHADER, sp)
  {
    window = create_window();
  }

  ShaderCompiler::~ShaderCompiler()
  {
    glfwDestroyWindow((GLFWwindow*)window);
    glfwTerminate();
  }

  bool ShaderCompiler::compile(Work &w)
  {
    u32 vert_index = 0u,
      frag_index = 0u,
      program_index = 0u;

    glfwMakeContextCurrent((GLFWwindow*)window);

    if (!load_json(w)) return false;

     bool result = _compile(w, jsn, vert_index, frag_index, program_index, *a);

    // Cleanup
    if (vert_index){
      glDetachShader(program_index, vert_index);
      glDeleteShader(vert_index);
    }
    if (frag_index){
      glDetachShader(program_index, frag_index);
      glDeleteShader(frag_index);
    }

    if (vert_index && frag_index)
      glDeleteProgram(program_index);

    glfwSwapBuffers((GLFWwindow*)window);
    glfwPollEvents();

    return result;
  }
}