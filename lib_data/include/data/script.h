#pragma once

#include <xstring>

#include <runtime/trace.h>
#include <runtime/assert.h>
#include <runtime/types.h>

#include <angelscript.h>
#include "angelscript/scriptstdstring.h"
#include "angelscript/scriptbuilder.h"

namespace pge
{
  namespace script
  {
    namespace API
    {
      const char *DCL_CREATE_WORLD  = "uint64 create_world (int32 num_layers)";
      const char *DCL_RELEASE_WORLD = "void release_world (uint64 world)";

      u64 (*create_world) (i32 num_layers);
      void release_world (u64 world);
    }

    namespace DEBUG
    {
      const char *DCL_PRINT = "void pge_print(const string &in)";
      void print(const std::string &msg)
      {
        LOG("%s\n", msg.c_str());
      }
    }

    //-----------------------------------------

    class CBytecodeStream : public asIBinaryStream
    {
    public:
      CBytecodeStream(FILE *fp) : f(fp) {}
      void Write(const void *ptr, asUINT size) 
      {
        if( size == 0 ) return; 
        fwrite(ptr, size, 1, f); 
      }
      void Read(void *ptr, asUINT size) 
      { 
        if( size == 0 ) return; 
        fread(ptr, size, 1, f); 
      }
    protected:
      FILE *f;
    };
    
    typedef void (*cb_engine) (const asSMessageInfo *msg, void *param);

    void cb_engine_message(const asSMessageInfo *msg, void *param)
    {
      const char *type = "ERR ";
      if( msg->type == asMSGTYPE_WARNING ) 
        type = "WARN";
      else if( msg->type == asMSGTYPE_INFORMATION ) 
        type = "INFO";
      LOG("%s : %s\n\t(%d, %d) : %s", type, msg->message, msg->row, msg->col, msg->section);
      param = param;
    }

    static inline asIScriptEngine *get_engine(const cb_engine cb_message = cb_engine_message)
    {
      i32 r;

      asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

      // Setups message callback
      r = engine->SetMessageCallback(asFUNCTION(cb_message), 0, asCALL_CDECL); 
      XASSERT( r >= 0 , "Can't register engine's message callback, err:%d.", r );

      // Registers custom types
      RegisterStdString(engine);

      // Registers dev func
      r = engine->RegisterGlobalFunction(DEBUG::DCL_PRINT, asFUNCTION(DEBUG::print), asCALL_CDECL); 
      ASSERT( r >= 0 );

      return engine;
    }

  }
}