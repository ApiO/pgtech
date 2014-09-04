#pragma once

#include <runtime/types.h>

namespace pge
{
  namespace window
  {
    void initialize   (void); 
    void shutdown     (void);
    f64  get_time     (void);
    void poll_events  (void);
    void swap_buffer  (void);
    bool should_close (void);
    void make_current_context (void);
    void set_swap_interval    (u32 value);

#if APP_SETUP_INFOS
    void print_info(void);
#endif
  }
}