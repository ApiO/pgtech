#include <windows.h>
#if APP_SETUP_INFOS
  #include <GL/glew.h>
  #include <GL/GL.h>
#endif
#include <GLFW/glfw3.h>

#include <runtime/trace.h>
#include <runtime/assert.h>
#include <engine/pge.h>
#include <application.h>

#include <input_system.h>
#include "window.h"

namespace
{
  using namespace pge;

  static GLFWwindow      *_window  = NULL;
  static WindowResizeFun  _window_resize_func = NULL;

  static void glfw_error_callback(int error, const char* description)
  {
    LOG("GLFW error (%d): %s", error, description);
  }

  static void glfw_on_window_resize(GLFWwindow*, i32 width, i32 height)
  {
    _window_resize_func(width, height);
  }


#if APP_SETUP_INFOS
  typedef struct
  {
    int attrib;
    const char* ext;
    const char* name;
  } AttribGL;

  typedef struct
  {
    int attrib;
    const char* name;
  } AttribGLFW;

  const AttribGL GL_ATTRIBS[] =
  {
    { GL_RED_BITS, NULL, "red bits" },
    { GL_GREEN_BITS, NULL, "green bits" },
    { GL_BLUE_BITS, NULL, "blue bits" },
    { GL_ALPHA_BITS, NULL, "alpha bits" },
    { GL_DEPTH_BITS, NULL, "depth bits" },
    { GL_STENCIL_BITS, NULL, "stencil bits" },
    { GL_STEREO, NULL, "stereo" },
    { GL_SAMPLES_ARB, "GL_ARB_multisample", "FSAA samples" },
    { 0, NULL, NULL }
  };

  const AttribGLFW GLFW_ATTRIBS[] =
  {
    { GLFW_CONTEXT_VERSION_MAJOR, "Context version major" },
    { GLFW_CONTEXT_VERSION_MINOR, "Context version minor" },
    { GLFW_OPENGL_FORWARD_COMPAT, "OpenGL forward compatible" },
    { GLFW_OPENGL_DEBUG_CONTEXT, "OpenGL debug context" },
    { GLFW_OPENGL_PROFILE, "OpenGL profile" },
    { GLFW_ICONIFIED, "Window is iconified" },
    { GLFW_VISIBLE, "Window is visible" },
    { GLFW_RESIZABLE, "Window is resizable" },
    { GLFW_DECORATED, "Window is decorated" },
    { 0, NULL }
  };
#endif
}


namespace pge
{
  namespace window
  {
    void initialize(void)
    {
      glfwSetErrorCallback(glfw_error_callback);

      int r = glfwInit();
      XASSERT(r, "ERROR: GLFW init error.");
    }

    void shutdown()
    {
      if (app->window) destroy();
      glfwTerminate();
    }

    void create(const char *title, i32 width, i32 height, bool full_screen, bool resizable)
    {
      if (app->window) destroy();

      /*
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
      //*/
      glfwWindowHint(GLFW_RESIZABLE, !full_screen && resizable ? GL_TRUE : GL_FALSE);

      _window = glfwCreateWindow(width, height, title, full_screen ? glfwGetPrimaryMonitor() : NULL, NULL);
      XASSERT(_window, "ERROR: GLFW window creation.");

      app->window = _window;
      
      input_system::init(_window, memory_globals::default_allocator());

      //glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }

    void destroy()
    {
      input_system::shutdown();

      glfwDestroyWindow(_window);
      app->window = NULL;
    }

    f64 get_time(void)
    {
      return glfwGetTime();
    }

    void poll_events()
    {
      glfwPollEvents();
    }

    void get_screen_resolution(i32 &width, i32 &height)
    {
      GLFWmonitor *mon = glfwGetPrimaryMonitor();
      const GLFWvidmode *mode = glfwGetVideoMode(mon);
      width = mode->width;
      height = mode->height;
    }

    void set_on_resized(WindowResizeFun func)
    {
      if (!func){
        glfwSetWindowSizeCallback(_window, NULL);
        return;
      }

      _window_resize_func = func;
      glfwSetWindowSizeCallback(_window, glfw_on_window_resize);
    }

    void make_current_context()
    {
      glfwMakeContextCurrent(_window);
    }

    void set_swap_interval(u32 value)
    {
      glfwSwapInterval(value);
    }

    void swap_buffer(void)
    {
      glfwSwapBuffers(_window);
    }

    bool should_close(void)
    {
      return glfwWindowShouldClose(_window) != 0;
    }

    void get_resolution(i32 &width, i32 &height)
    {
      glfwGetFramebufferSize(_window, &width, &height);
    }

    void get_size(i32 &width, i32 &height)
    {
      glfwGetWindowSize(_window, &width, &height);
    }

    void set_size(i32 &width, i32 &height)
    {
      glfwSetWindowSize(_window, width, height);
    }

    void set_title(const char *title)
    {
      glfwSetWindowTitle(_window, title);
    }

    void display_cursor(bool display)
    {
      glfwSetInputMode(_window, GLFW_CURSOR, display ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    }


#if APP_SETUP_INFOS
    void print_info()
    {
      i32 width, height;

      glfwGetWindowSize(_window, &width, &height);

      OUTPUT("\nGLFW infos:");

      OUTPUT("\tFramebuffer size: %ix%i", width, height);

      for (i32 i = 0; GLFW_ATTRIBS[i].name; i++)
      {
        OUTPUT("\t\t%s: %i", GLFW_ATTRIBS[i].name, glfwGetWindowAttrib(_window, GLFW_ATTRIBS[i].attrib));
      }

      OUTPUT("\tGL extensions supported:");
      for (i32 i = 0; GL_ATTRIBS[i].name; i++)
      {
        GLint value = 0;

        if (GL_ATTRIBS[i].ext)
        {
          if (!glfwExtensionSupported(GL_ATTRIBS[i].ext))
            continue;
        }

        glGetIntegerv(GL_ATTRIBS[i].attrib, &value);

        OUTPUT("\t\t%s: %i", GL_ATTRIBS[i].name, value);
      }
    }
#endif
  }
}