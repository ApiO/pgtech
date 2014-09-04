#ifdef GL_ERROR
#define OGL_DEBUG_HEADER
#endif

#ifdef APP_SETUP_INFOS
#define OGL_DEBUG_HEADER
#endif

#ifdef OGL_DEBUG_HEADER
  #include <windows.h>
  #include <GL/glew.h>
  #include <GL/GL.h>
  #include <stdio.h>

  #include <runtime/types.h>
  #include <runtime/trace.h>

  #include "ogl_debug.h"
#endif


#ifdef GL_ERROR
namespace pge
{

  const char *gl_error_type[] =
  {
    "",
    "GL_INVALID_ENUM",
    "GL_INVALID_VALUE",
    "GL_INVALID_OPERATION",
    "GL_INVALID_FRAMEBUFFER_OPERATION",
    "GL_OUT_OF_MEMORY",
    "GL_STACK_UNDERFLOW",
    "GL_STACK_OVERFLOW"
  };

  void _print_gl_last_error(char *file, unsigned line)
  {
    GLenum error_code = glGetError();
    u32 code;
    char *msg = NULL;
    switch (error_code)
    {
      case GL_NO_ERROR: return;
      case GL_INVALID_ENUM:
        msg = "An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.";
        code = 1;
        break;
      case GL_INVALID_VALUE:
        msg = "A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.";
        code = 2;
        break;
      case GL_INVALID_OPERATION:
        msg = "The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.";
        code = 3;
        break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
        msg = "The framebuffer object is not complete. The offending command is ignored and has no other side effect than to set the error flag.";
        code = 4;
        break;
      case GL_OUT_OF_MEMORY:
        msg = "There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
        code = 5;
        break;
      case GL_STACK_UNDERFLOW:
        msg = "An attempt has been made to perform an operation that would cause an internal stack to underflow.";
        code = 6;
        break;
      case GL_STACK_OVERFLOW:
        msg = "An attempt has been made to perform an operation that would cause an internal stack to overflow.";
        code = 7;
        break;
      default:
        sprintf(msg, "Unhandled error code %d", error_code);
        code = 0;
    }
    OUTPUT("\r\nat %s:(%d)\r\n\tOpenGl error %s\r\n\t%s\r\n", file, line, gl_error_type[code], msg, line);
  }
}
#endif


#ifdef APP_SETUP_INFOS
namespace pge
{
  //--------------------------------------------------------------------------
  //    OPENGL
  //--------------------------------------------------------------------------

  const char* GL_TYPE_TO_STRING(const u32 &type)
  {
    if (GL_FLOAT == type) {
      return "GL_FLOAT";
    }
    if (GL_FLOAT_VEC2 == type) {
      return "GL_FLOAT_VEC2";
    }
    if (GL_FLOAT_VEC3 == type) {
      return "GL_FLOAT_VEC3";
    }
    if (GL_FLOAT_VEC4 == type) {
      return "GL_FLOAT_VEC4";
    }
    if (GL_FLOAT_MAT2 == type) {
      return "GL_FLOAT_MAT2";
    }
    if (GL_FLOAT_MAT3 == type) {
      return "GL_FLOAT_MAT3";
    }
    if (GL_FLOAT_MAT4 == type) {
      return "GL_FLOAT_MAT4";
    }
    if (GL_INT == type) {
      return "GL_INT";
    }
    if (GL_BOOL == type) {
      return "GL_BOOL";
    }
    if (GL_SAMPLER_2D == type) {
      return "GL_SAMPLER_2D";
    }
    if (GL_SAMPLER_3D == type) {
      return "GL_SAMPLER_3D";
    }
    if (GL_SAMPLER_CUBE == type) {
      return "GL_SAMPLER_CUBE";
    }
    if (GL_SAMPLER_2D_SHADOW == type) {
      return "GL_SAMPLER_2D_SHADOW";
    }
    return "OTHER";
  }

  const GLenum GL_PARAMS[] ={
    GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
    GL_MAX_CUBE_MAP_TEXTURE_SIZE,
    GL_MAX_DRAW_BUFFERS,
    GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,
    GL_MAX_TEXTURE_UNITS,
    GL_MAX_TEXTURE_IMAGE_UNITS,
    GL_MAX_TEXTURE_SIZE,
    GL_MAX_VARYING_FLOATS,
    GL_MAX_VERTEX_ATTRIBS,
    GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
    GL_MAX_VERTEX_UNIFORM_COMPONENTS,
    GL_MAX_VERTEX_UNIFORM_BLOCKS,
    GL_MAX_GEOMETRY_UNIFORM_BLOCKS, 
    GL_MAX_FRAGMENT_UNIFORM_BLOCKS, 
    GL_MAX_UNIFORM_BLOCK_SIZE,
    GL_MAX_FRAMEBUFFER_HEIGHT,
    GL_MAX_FRAMEBUFFER_LAYERS,
    GL_MAX_FRAMEBUFFER_SAMPLES,
    GL_MAX_FRAMEBUFFER_WIDTH,
    GL_MAX_VIEWPORT_DIMS,
    GL_STEREO
  };

  const char* GL_PARAM_NAMES[] ={
    "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS",
    "GL_MAX_CUBE_MAP_TEXTURE_SIZE",
    "GL_MAX_DRAW_BUFFERS",
    "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS",
    "GL_MAX_TEXTURE_UNITS",
    "GL_MAX_TEXTURE_IMAGE_UNITS",
    "GL_MAX_TEXTURE_SIZE",
    "GL_MAX_VARYING_FLOATS",
    "GL_MAX_VERTEX_ATTRIBS",
    "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS",
    "GL_MAX_VERTEX_UNIFORM_COMPONENTS",
    "GL_MAX_VERTEX_UNIFORM_BLOCKS",
    "GL_MAX_GEOMETRY_UNIFORM_BLOCKS",
    "GL_MAX_FRAGMENT_UNIFORM_BLOCKS",
    "GL_MAX_UNIFORM_BLOCK_SIZE",
    "GL_MAX_FRAMEBUFFER_HEIGHT",
    "GL_MAX_FRAMEBUFFER_LAYERS",
    "GL_MAX_FRAMEBUFFER_SAMPLES",
    "GL_MAX_FRAMEBUFFER_WIDTH",
    "GL_MAX_VIEWPORT_DIMS",
    "GL_STEREO"
  };

  void log_gl_params()
  {
    OUTPUT("GL Context Params:");
    char msg[256];
    // integers - only works if the order is 0-10 integer return types
    for (int i = 0; i < 19; i++) {
      int v = 0;
      glGetIntegerv(GL_PARAMS[i], &v);
      sprintf(msg, "\t%s %i", GL_PARAM_NAMES[i], v);
      OUTPUT(msg);
    }

    // others
    int v[2];
    v[0] = v[1] = 0;
    glGetIntegerv(GL_PARAMS[19], v);
    sprintf(msg, "\t%s %i %i", GL_PARAM_NAMES[19], v[0], v[1]);
    OUTPUT(msg);

    unsigned char s = 0;
    glGetBooleanv(GL_PARAMS[20], &s);
    sprintf(msg, "\t%s %i", GL_PARAM_NAMES[20], (unsigned int)s);
    OUTPUT(msg);
  }

  void print_gl_version_info()
  {
    // get version info
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION);   // version as a string
    OUTPUT("\nRenderer: %s\nOpenGL version supported %s\n", renderer, version);
  }

  //--------------------------------------------------------------------------
  //    GLFW
  //--------------------------------------------------------------------------



  //--------------------------------------------------------------------------
  //    SHADER 
  //--------------------------------------------------------------------------

  void print_program_info_output(u32 program_id)
  {
    const i32 max_length = 2048;
    int actual_length = 0;
    char log[max_length];

    glGetProgramInfoLog(program_id, max_length, &actual_length, log);

    OUTPUT("Shader program [%i] info log: %s\n", program_id, strlen(log) ? log : "OK");
  }

  bool check_linking(u32 program_id)
  {
    int params = -1;

    glGetProgramiv(program_id, GL_LINK_STATUS, &params);

    if (GL_TRUE != params)
    {
      const i32 max_length = 2048;
      i32 actual_length = 0;
      char log[max_length];

      glGetProgramInfoLog(program_id, max_length, &actual_length, log);

      OUTPUT("ERROR: could not link shader program GL index %i\n%s\n",
             program_id, strlen(log) ? log : "none");

      return false;
    }
    return true;
  }

  bool program_is_valid(u32 program_id)
  {
    glValidateProgram(program_id);

    int params = -1;
    glGetProgramiv(program_id, GL_VALIDATE_STATUS, &params);

    OUTPUT("\nShader program [%i] GL_VALIDATE_STATUS = %s",
           program_id, params ? "GL_TRUE" : "GL_FALSE");

    if (GL_TRUE != params)
    {
      print_program_info_output(program_id);
      return false;
    }

    return true;
  }

  void print_all(u32 program_id,
                        u32 vs, const char *vs_name,
                        u32 fs, const char *fs_name)
  {
    OUTPUT("\nShader program [%i] info:", program_id);
    int params = -1;
    glGetProgramiv(program_id, GL_LINK_STATUS, &params);

    char value[32];
    strcpy(value, (GL_TRUE == params) ? "GL_TRUE" : "GL_FALSE");

    OUTPUT("\tGL_LINK_STATUS = %s", value);

    glGetProgramiv(program_id, GL_ATTACHED_SHADERS, &params);
    OUTPUT("\tGL_ATTACHED_SHADERS = %i", params);

    if (vs) OUTPUT("\t\tvertex shader index [%i]. file name: %s", vs, vs_name);
    if (fs) OUTPUT("\t\tfragment shader index [%i]. file name: %s", fs, fs_name);

    glGetProgramiv(program_id, GL_ACTIVE_ATTRIBUTES, &params);
    OUTPUT("\tGL_ACTIVE_ATTRIBUTES = %i", params);

    for (i32 i = 0; i < params; i++)
    {
      char name[64];
      i32 max_length = 64;
      i32 actual_length = 0;
      i32 size = 0;
      GLenum type;

      glGetActiveAttrib(program_id, i, max_length, &actual_length, &size, &type, name);
      if (size > 1)
      {
        for (int j = 0; j < size; j++)
        {
          char long_name[64];
          sprintf(long_name, "%s[%i]", name, j);
          int location = glGetAttribLocation(program_id, long_name);
          OUTPUT("\t\t%i) type:%s name:%s location:%i",
                 i, GL_TYPE_TO_STRING(type), long_name, location);
        }
      }
      else {
        int location = glGetAttribLocation(program_id, name);
        OUTPUT("\t\t%i) type:%s name:%s location:%i",
               i, GL_TYPE_TO_STRING(type), name, location);
      }
    }

    glGetProgramiv(program_id, GL_ACTIVE_UNIFORMS, &params);
    OUTPUT("\tGL_ACTIVE_UNIFORMS = %i", params);

    for (i32 i = 0; i < params; i++)
    {
      char name[64];
      i32 max_length = 64;
      i32 actual_length = 0;
      i32 size = 0;
      GLenum type;

      glGetActiveUniform(program_id, i, max_length, &actual_length, &size, &type, name);
      if (size > 1)
      {
        for (int j = 0; j < size; j++)
        {
          char long_name[64];
          sprintf(long_name, "%s[%i]", name, j);
          int location = glGetUniformLocation(program_id, long_name);
          OUTPUT("\t\t%i) type:%s name:%s location:%i",
                 i, GL_TYPE_TO_STRING(type), long_name, location);
        }
      }
      else {
        int location = glGetUniformLocation(program_id, name);
        OUTPUT("\t\t%i) type:%s name:%s location:%i",
               i, GL_TYPE_TO_STRING(type), name, location);
      }
    }

    print_program_info_output(program_id);
  }

}
#endif