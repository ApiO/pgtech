#pragma once

namespace pge
{
#ifndef GL_ERROR
#define PRINT_GL_LAST_ERROR() (void(0))
#else
  void _print_gl_last_error(char *file, unsigned line);
#define PRINT_GL_LAST_ERROR()\
  MULTI_LINE_MACRO_BEGIN \
  _print_gl_last_error(__FILE__, __LINE__); \
  MULTI_LINE_MACRO_END
#endif

#ifdef APP_SETUP_INFOS
  void log_gl_params();
  void print_gl_version_info();
  bool check_linking(u32 program_id);
  bool program_is_valid(u32 program_id);
  void print_all(u32 program_id,
                 u32 vs, const char *vs_name,
                 u32 fs, const char *fs_name);
#endif
}