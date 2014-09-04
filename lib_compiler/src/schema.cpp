#include <runtime/json.h>
#include <runtime/trace.h>
#include <runtime/temp_allocator.h>
#include <runtime/string_stream.h>

#include "schema.h"

namespace pge
{
  using namespace pge::string_stream;

  static Allocator *allocator = NULL;

  static Json *compilers_config_schema = NULL;
  static Json *settings_config_schema  = NULL;

  static Json *physics_schema  = NULL;
  static Json *package_schema  = NULL;
  static Json *animset_schema  = NULL;
  static Json *actor_schema    = NULL;
  static Json *shape_schema    = NULL;
  static Json *sprite_schema   = NULL;
  static Json *texture_schema  = NULL;
  static Json *unit_schema     = NULL;
  static Json *shader_schema   = NULL;
  static Json *level_schema    = NULL;
  static Json *audio_schema    = NULL;
  static Json *sound_schema    = NULL;
  static Json *particle_schema = NULL;

  bool load_schemas(const char *schemas_folder, Allocator &a, StringPool &sp)
  {
    allocator = &a;

    compilers_config_schema = MAKE_NEW(a, Json, a, sp);
    settings_config_schema  = MAKE_NEW(a, Json, a, sp);

    physics_schema  = MAKE_NEW(a, Json, a, sp);
    package_schema  = MAKE_NEW(a, Json, a, sp);
    animset_schema  = MAKE_NEW(a, Json, a, sp);
    actor_schema    = MAKE_NEW(a, Json, a, sp);
    shape_schema    = MAKE_NEW(a, Json, a, sp);
    sprite_schema   = MAKE_NEW(a, Json, a, sp);
    texture_schema  = MAKE_NEW(a, Json, a, sp);
    unit_schema     = MAKE_NEW(a, Json, a, sp);
    shader_schema   = MAKE_NEW(a, Json, a, sp);
    level_schema    = MAKE_NEW(a, Json, a, sp);
    audio_schema    = MAKE_NEW(a, Json, a, sp);
    sound_schema    = MAKE_NEW(a, Json, a, sp);
    particle_schema = MAKE_NEW(a, Json, a, sp);

    char path[MAX_PATH];
    strcpy(path, schemas_folder);
    if (path[strlen(path) - 1] != '\\')
      strcat(path, "\\");

    char filepath[MAX_PATH];

    // config files

    sprintf(filepath, "%s%s.schema", path, "compilers_config");
    if (!json::parse_from_file(*compilers_config_schema, json::root(*compilers_config_schema), filepath)) return false;

    sprintf(filepath, "%s%s.schema", path, "settings_config");
    if (!json::parse_from_file(*settings_config_schema, json::root(*settings_config_schema), filepath)) return false;

    // Resources

    sprintf(filepath, "%s%s.schema", path, "physics");
    if (!json::parse_from_file(*physics_schema, json::root(*physics_schema), filepath)) return false;

    sprintf(filepath, "%s%s.schema", path, "package");
    if (!json::parse_from_file(*package_schema, json::root(*package_schema), filepath)) return false;

    sprintf(filepath, "%s%s.schema", path, "animset");
    if (!json::parse_from_file(*animset_schema, json::root(*animset_schema), filepath)) return false;

    sprintf(filepath, "%s%s.schema", path, "actor");
    if (!json::parse_from_file(*actor_schema, json::root(*actor_schema), filepath)) return false;

    sprintf(filepath, "%s%s.schema", path, "shape");
    if (!json::parse_from_file(*shape_schema, json::root(*shape_schema), filepath)) return false;

    sprintf(filepath, "%s%s.schema", path, "sprite");
    if (!json::parse_from_file(*sprite_schema, json::root(*sprite_schema), filepath)) return false;

    sprintf(filepath, "%s%s.schema", path, "texture");
    if (!json::parse_from_file(*texture_schema, json::root(*texture_schema), filepath)) return false;

    sprintf(filepath, "%s%s.schema", path, "unit");
    if (!json::parse_from_file(*unit_schema, json::root(*unit_schema), filepath)) return false;

    sprintf(filepath, "%s%s.schema", path, "shader");
    if (!json::parse_from_file(*shader_schema, json::root(*shader_schema), filepath)) return false;

    sprintf(filepath, "%s%s.schema", path, "level");
    if (!json::parse_from_file(*level_schema, json::root(*level_schema), filepath)) return false;

    sprintf(filepath, "%s%s.schema", path, "audio");
    if (!json::parse_from_file(*audio_schema, json::root(*audio_schema), filepath)) return false;

    sprintf(filepath, "%s%s.schema", path, "sound");
    if (!json::parse_from_file(*sound_schema, json::root(*sound_schema), filepath)) return false;

    sprintf(filepath, "%s%s.schema", path, "particle");
    if (!json::parse_from_file(*particle_schema, json::root(*sound_schema), filepath)) return false;

    return true;
  }

  void unload_schemas(void)
  {
    MAKE_DELETE((*allocator), Json, compilers_config_schema);
    MAKE_DELETE((*allocator), Json, settings_config_schema);

    MAKE_DELETE((*allocator), Json, package_schema);
    MAKE_DELETE((*allocator), Json, animset_schema);
    MAKE_DELETE((*allocator), Json, physics_schema);
    MAKE_DELETE((*allocator), Json, actor_schema);
    MAKE_DELETE((*allocator), Json, shape_schema);
    MAKE_DELETE((*allocator), Json, sprite_schema);
    MAKE_DELETE((*allocator), Json, texture_schema);
    MAKE_DELETE((*allocator), Json, unit_schema);
    MAKE_DELETE((*allocator), Json, shader_schema);
    MAKE_DELETE((*allocator), Json, level_schema);
    MAKE_DELETE((*allocator), Json, audio_schema);
    MAKE_DELETE((*allocator), Json, sound_schema);
    MAKE_DELETE((*allocator), Json, particle_schema);
  }

  // resource validation

  bool validate_resource(Json &jsn, const ResourceType type, const char *file)
  {
    Json *schema = NULL;

    switch (type)
    {
    case RESOURCE_TYPE_PACKAGE:  schema = package_schema;  break;
    case RESOURCE_TYPE_UNIT:     schema = unit_schema;     break;
    case RESOURCE_TYPE_PHYSICS:  schema = physics_schema;  break;
    case RESOURCE_TYPE_ACTOR:    schema = actor_schema;    break;
    case RESOURCE_TYPE_SHAPE:    schema = shape_schema;    break;
    case RESOURCE_TYPE_ANIMSET:  schema = animset_schema;  break;
    case RESOURCE_TYPE_SPRITE:   schema = sprite_schema;   break;
    case RESOURCE_TYPE_TEXTURE:  schema = texture_schema;  break;
    case RESOURCE_TYPE_SHADER:   schema = shader_schema;   break;
    case RESOURCE_TYPE_LEVEL:    schema = level_schema;    break;
    case RESOURCE_TYPE_AUDIO:    schema = audio_schema;    break;
    case RESOURCE_TYPE_SOUND:    schema = sound_schema;    break;
    case RESOURCE_TYPE_PARTICLE: schema = particle_schema; break;
    default:
      LOG("Json schema validation : resource type %d has no schema, file: %s", (i32)type, file);
      return false;
    }

    if (!json::validate(jsn, *schema))
    {
      TempAllocator4096 ta(memory_globals::default_allocator());
      Array<char*> errors(ta);
      Buffer error_string(ta);

      json::get_last_errors(jsn, errors);
      for (u32 i = 0; i < array::size(errors); i++)
        error_string << "\r\n\t" << errors[i];

      LOG("Json schema validation failed\n\tfile: \"%s\" : %s", file, c_str(error_string));
      return false;
    }

    return true;
  }

  // compile config validation

  bool validate_compile_config(Json &jsn)
  {

    if (!json::validate(jsn, *compilers_config_schema))
    {
      TempAllocator4096 ta(memory_globals::default_allocator());
      Array<char*> errors(ta);
      Buffer error_string(ta);

      json::get_last_errors(jsn, errors);
      for (u32 i = 0; i < array::size(errors); i++)
        error_string << "\r\n\t" << errors[i];

      LOG("Compile config validation failed: %s", c_str(error_string));
      return false;
    }
    return true;
  }

  bool validate_project_compilers_config(Json &jsn, const char *src)
  {
    if (!json::validate(jsn, *compilers_config_schema))
    {
      TempAllocator4096 ta(memory_globals::default_allocator());
      Array<char*> errors(ta);
      Buffer error_string(ta);

      json::get_last_errors(jsn, errors);
      for (u32 i = 0; i < array::size(errors); i++)
        error_string << "\r\n\t" << errors[i];

      LOG("Project compile config validation failed\n\tfile: \"%s\" : %s", src, c_str(error_string));
      return false;
    }
    return true;
  }

  // settings config validation

  bool validate_project_settings_config(Json &jsn, const char *src)
  {
    if (!json::validate(jsn, *settings_config_schema))
    {
      LOG("Project settings.config json validation failed : %s", src);
      return false;
    }
    return true;
  }

}