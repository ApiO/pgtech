#include <windows.h>
#include <stdio.h>
#include <algorithm>
#include <string>

#include <mintomic/mintomic.h>

#include <runtime/assert.h>
#include <runtime/trace.h>
#include <runtime/pool_allocator.h>
#include <runtime/array.h>
#include <runtime/string_pool.h>
#include <runtime/file_system.h>
#include <runtime/timer.h>
#include <runtime/tinycthread.h>

#include <compiler/compile_manager.h>

#include "application.h"

namespace
{
  using namespace pge;

  typedef mint_atomic32_t mint32;

  struct Command
  {
    enum Type
    {
      BUILD, REBUILD, START_WATCH, STOP_WATCH
    };
    Command(u64 p, Type t) : project(p), type(t){}
    u64  project;
    Type type;
  };

  struct Application
  {
    Application(Allocator &_a) : a(&_a), commands(_a){}
    Array<Command> commands;
    Allocator     *a;
    thrd_t         compile_manager_thread;
    mint32         should_quit;
    mtx_t          cmd_mutex;
  };

  static Application *app = NULL;

  u64 project;

  static char cfg[MAX_PATH];
  static char src[MAX_PATH];
  static char data[MAX_PATH];
  static char sch[MAX_PATH];
  const char *header = "[SERVER]";
  const char *LINE = "----------------------------------------------";
  const char *LOG_FILE_NAME = "log.txt";

  static void output_handler(const char *msg)
  {
    printf(msg);
  }

  static void log_handler(const char *msg)
  {
    FILE *stream = fopen(LOG_FILE_NAME, "ab");
    fwrite(msg, strlen(msg), 1, stream);
    fclose(stream);
  }

  const char *ARG_LOG  = "-l";    //log output to file text log.txt
  const char *ARG_MUTE = "-m";    //no output
  const char *ARG_CFG  = "cfg:";  //follow by configuration file's folder
  const char *ARG_SRC  = "src:";  //follow by project's sources folder
  const char *ARG_DATA = "data:"; //follow by project's data folder
  const char *ARG_SCH  = "sch:";  //follow by schemas file's folder

  const DWORD CONSOLE_FONT_COLOR_INIT = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
  const DWORD CONSOLE_FONT_COLOR_EVENT = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
  const DWORD CONSOLE_FONT_COLOR_RESET = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

  static char* get_cmd_option(char ** begin, char ** end, const std::string & option)
  {
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) {
      return *itr;
    }
    return 0;
  }

  static bool cmd_option_exists(char** begin, char** end, const std::string& option)
  {
    return std::find(begin, end, option) != end;
  }

  static void handle_cmd_args(int argc, char * argv[], char *cfg, char *sch, char *src, char *data)
  {
    // Default project src/data folder
    if (cmd_option_exists(argv, argv + argc, ARG_SRC) && cmd_option_exists(argv, argv + argc, ARG_DATA)) {
      char *path = get_cmd_option(argv, argv + argc, ARG_SRC);
      XASSERT(path, "Argument \"%s\" not found", ARG_SRC);
      XASSERT(file_system::directory_exists(path), "Folder not found. arg: \"%s\" path: \"%s\"", ARG_SRC, path);
      strcpy(src, path);

      path = get_cmd_option(argv, argv + argc, ARG_DATA);
      XASSERT(path, "Argument \"%s\" not found", ARG_DATA);
      XASSERT(file_system::directory_exists(path), "Folder not found. arg: \"%s\" path: \"%s\"", ARG_DATA, path);
      strcpy(data, path);
    } else {
      strcpy(src, "..\\sandbox\\src\\resources\0");
      strcpy(data, "..\\sandbox\\src\\data\0");
    }

    // compiler config folder
    if (cmd_option_exists(argv, argv + argc, ARG_CFG)) {
      char *path = get_cmd_option(argv, argv + argc, ARG_CFG);
      XASSERT(path, "Argument \"%s\" not found", ARG_CFG);
      XASSERT(file_system::directory_exists(path), "Folder not found. arg: \"%s\" path: \"%s\"", ARG_CFG, path);
      strcpy(cfg, path);
    } else {
      strcpy(cfg, "data");
    }

    // schemas folder
    if (cmd_option_exists(argv, argv + argc, ARG_SCH)) {
      char *path = get_cmd_option(argv, argv + argc, ARG_SCH);
      XASSERT(path, "Argument \"%s\" not found", ARG_SCH);
      XASSERT(file_system::directory_exists(path), "Folder not found. arg: \"%s\" path: \"%s\"", ARG_SCH, path);
      strcpy(sch, path);
    } else {
      strcpy(sch, "schemas");
    }

    if (cmd_option_exists(argv, argv + argc, ARG_MUTE)) {
      // mute logs
      pge::set_output_handler(NULL);
      pge::set_log_handler(NULL);
    } else if (cmd_option_exists(argv, argv + argc, ARG_LOG)) {
      // log to file activation
      if (file_system::file_exists(LOG_FILE_NAME))
        file_system::delete_file(LOG_FILE_NAME);

      pge::set_output_handler(output_handler);
      pge::set_log_handler(log_handler);
    } else {
      pge::set_output_handler(output_handler);
      pge::set_log_handler(output_handler);
    }
  }

  static void _build(void)
  {
    Timer timer;

    //sets the color to intense red on blue background
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_EVENT);

    OUTPUT("%s Build project ID:%llu", header, project);

    //reverting back to the normal color
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_RESET);


    start_timer(timer);
    compile_manager::build(project);

    stop_timer(timer);

    //sets the color to intense red on blue background
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_EVENT);

    OUTPUT("%s project ID:%llu build end, in %.3f sec",
           header,
           project,
           get_elapsed_time_in_sec(timer));

    //reverting back to the normal color
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_RESET);
  }

  static void _rebuild(void)
  {
    Timer timer;

    //sets the color to intense red on blue background
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_EVENT);

    //reverting back to the normal color
    OUTPUT("%s Rebuild project ID:%llu", header, project);

    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_RESET);

    start_timer(timer);
    compile_manager::rebuild(project);

    stop_timer(timer);

    //sets the color to intense red on blue background
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_EVENT);

    OUTPUT("%s project ID:%llu rebuild end, in %.3f sec",
           header,
           project,
           get_elapsed_time_in_sec(timer));

    //reverting back to the normal color
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_RESET);

  }


  static int compile_manager_func(void *arg)
  {
    (void)arg;

    Timer timer;
    bool state;

    //sets the color to intense red on blue background
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_INIT);
    OUTPUT("%s Initialize compile manager\n\tcfg: \"%s\"\n\tsch: \"%s\"", header, cfg, sch);
    //reverting back to the normal color
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_RESET);


    start_timer(timer);
    state = compile_manager::init(cfg, sch, *app->a);
    stop_timer(timer);


    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_INIT);
    OUTPUT("\t\"%s\" in %.3f ms", state ? "SUCCESS" : "FAIL", get_elapsed_time_in_ms(timer));
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_RESET);

    if (!state) return -1;
    
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_INIT);
    OUTPUT("%s Register project\n\tsrc: \"%s\"\n\tdata: \"%s\"", header, src, data);
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_RESET);

    project = compile_manager::add(src, data);

    state = compile_manager::project_loaded(project);

    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_INIT);
    OUTPUT("\tProject ID:%llu \"%s\"", project, state ? "SUCCESS" : "FAIL");
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_RESET);

    if (!state) return -1;

    _build();

    //reverting back to the normal color
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_RESET);

    //OUTPUT("%s Start watching project ID:%llu", header, project);
    //compile_manager::start_watching(project);

    while (mint_load_32_relaxed(&app->should_quit) == 0u) {

      mtx_lock(&app->cmd_mutex);
      const u32 num_commands = array::size(app->commands);
      mtx_unlock(&app->cmd_mutex);

      if (!num_commands) {
        compile_manager::update();
        Sleep(10);
        continue;
      }

      mtx_lock(&app->cmd_mutex);
      Command cmd = array::pop_back(app->commands);
      mtx_unlock(&app->cmd_mutex);

      switch (cmd.type) {
      case Command::BUILD:
        _build();
        break;
      case Command::REBUILD:
        _rebuild();
        break;
      case Command::START_WATCH:
        OUTPUT("%s Start watching project ID:%llu", header, project);
        pge::compile_manager::start_watching(project);
        break;
      case Command::STOP_WATCH:
        OUTPUT("%s Stop watching project ID:%llu", header, project);
        pge::compile_manager::stop_watching(project);
        break;
      }
      Sleep(10);
    }

    //OUTPUT("%s Stop watching project ID:%llu", header, project);
    //compile_manager::stop_watching(project);

    compile_manager::remove(project);

    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_INIT);
    OUTPUT("%s Project ID %llu unloaded", header, project);
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_RESET);

    compile_manager::shutdown();

    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_INIT);
    OUTPUT("%s Compile manager shutdowned", header);
    SetConsoleTextAttribute(hStdout, CONSOLE_FONT_COLOR_RESET);

    return 0;
  }
}

namespace pge
{
  namespace application
  {
    int start(int argc, char * argv[])
    {
      memory_globals::init();
      Allocator &a = memory_globals::default_allocator();

      app = MAKE_NEW(a, Application, a);
      app->should_quit._nonatomic  = 0u;
      mtx_init(&app->cmd_mutex, mtx_plain);
      mint_thread_fence_release();
      
      handle_cmd_args(argc, argv, cfg, sch, src, data);

      sprintf(src,  "E:/[Proto_Gecko]/gonp/resources");
      sprintf(data, "E:/[Proto_Gecko]/gonp/data");

      OUTPUT("%s\n\t\tCompile Server\n%s", LINE, LINE);

      thrd_create(&app->compile_manager_thread, compile_manager_func, &app);

      return 0;
    }

    void build(void)
    {
      mtx_lock(&app->cmd_mutex);
      array::push_back(app->commands, Command(project, Command::BUILD));
      mtx_unlock(&app->cmd_mutex);
    }

    void rebuid(void)
    {
      mtx_lock(&app->cmd_mutex);
      array::push_back(app->commands, Command(project, Command::REBUILD));
      mtx_unlock(&app->cmd_mutex);
    }

    void start_watch(void)
    {
      mtx_lock(&app->cmd_mutex);
      array::push_back(app->commands, Command(project, Command::START_WATCH));
      mtx_unlock(&app->cmd_mutex);
    }

    void stop_watch(void)
    {
      mtx_lock(&app->cmd_mutex);
      array::push_back(app->commands, Command(project, Command::STOP_WATCH));
      mtx_unlock(&app->cmd_mutex);
    }

    void stop(void)
    {
      mint_store_32_relaxed(&app->should_quit, 1u);

      int result;
      thrd_join(app->compile_manager_thread, &result);

      mtx_destroy(&app->cmd_mutex);
      MAKE_DELETE((*app->a), Application, app);

      memory_globals::shutdown();
    }
  }
}