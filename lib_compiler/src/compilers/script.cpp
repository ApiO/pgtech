#include <runtime/types.h>
#include <runtime/trace.h>

#include <data/script.h>

#include "compiler_types.h"
#include "compiler.h"

namespace
{
  using namespace pge;
  using namespace pge::script;
  using namespace pge::string_stream;

  const char *MODULE_NAME = "default";

  struct UserData
  {
    UserData(Work *_w, Buffer *_buf)
      : w(_w), buf(_buf)  {}
    Work      *w;
    Buffer    *buf;
  };

  int cb_builder_include(const char *include, const char *from, CScriptBuilder *builder, void *user_data)
  {
    UserData &ud = *(UserData*)user_data;

    if (!compiler::load_dependency(*ud.w, RESOURCE_TYPE_SCRIPT, include, *ud.buf))
    {
      return -1;
    }

    i32 r = builder->AddSectionFromMemory(include, string_stream::c_str(*ud.buf));
    if (r < 0)
    {
      char path[MAX_PATH];
      sprintf(path, "%s\\%s.%s", ud.w->src, include, ResourceExtension[RESOURCE_TYPE_SPRITE]);

      LOG("ERROR: wrong syntax in the script in \"%s\", err:%d.", path, r);
      return -1;
    }

    from = from;

    return 0;
  }

}

namespace pge
{
  ScriptCompiler::ScriptCompiler(Allocator &a)
    : BinaryCompiler(a, RESOURCE_TYPE_SCRIPT) {}

  bool ScriptCompiler::compile(Work &w)
  {
    if (!load_bytes(w)) return false;

    i32 r;

    asIScriptEngine *engine = get_engine();

    CScriptBuilder builder;

    Buffer temp_buf(*a);
    UserData ud(&w, &temp_buf);

    builder.SetIncludeCallback(cb_builder_include, &ud);

    r = builder.StartNewModule(engine, MODULE_NAME);
    if (r < 0)
    {
      LOG("Unrecoverable error while starting a new module \"%s\", err:%d.", MODULE_NAME, r);
      engine->Release();
      return false;
    }

    r = builder.AddSectionFromMemory(w.src, string_stream::c_str(buf));
    if (r < 0)
    {
      LOG("Error in script \"%s\", err:%d.", w.src, r);
      engine->Release();
      return false;
    }

    // Builds script
    r = builder.BuildModule();
    if (r < 0)
    {
      LOG("ERROR: Build processfailed for script \"%s\", err:%d.", w.src, r);
      engine->Release();
      return false;
    }

    // Writes bytes into bin
    asIScriptModule *mod = engine->GetModule(MODULE_NAME);
    CBytecodeStream cbs(w.data);

    r = mod->SaveByteCode(&cbs);
    if (r < 0)
    {
      LOG("Error occured during script's byte saving process, err:%d.", r);
      engine->Release();
      return false;
    }

    // Clean up
    engine->Release();

    return true;
  }
}