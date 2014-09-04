#include <runtime/trace.h>
#include <runtime/assert.h>
#include <runtime/collection_types.h>
#include <runtime/array.h>
#include <runtime/hash.h>
#include <runtime/json.h>
#include <runtime/string_pool.h>
#include <runtime/murmur_hash.h>
#include <runtime/file_system.h>
#include <runtime/idlut.h>
#include <runtime/temp_allocator.h>
#include <runtime/timer.h>

#include <compiler/compile_manager.h>

#include "compilers/compiler_types.h"
#include "types.h"
#include "schema.h"
#include "linkage_manager.h"
#include "id_string_manager.h"

#define PGC_MTD_SIZE (sizeof(u64) + sizeof(Metadata))
#define PGC_DEP_SIZE (sizeof(u64) + sizeof(DataId))

namespace
{
  using namespace pge;
  using namespace pge::file_system;
  using namespace pge::string_stream;

  const char *SERVER_COMPILE_FILE_NAME = "server_compilers.pgconf";
  const char *COMPILE_FILE_NAME = "compilers.pgconf";
  const char *SETTINGS_FILE_NAME = "settings.pgconf";


  struct CompileContext
  {
    CompileContext(Allocator &a) :
      allocator(&a),
      projects(a),
      work_buf(a), sp(a),
      font_compiler(a),
      script_compiler(a),
      shader_compiler(a, sp),
      animset_compiler(a, sp),
      level_compiler(a, sp),
      package_compiler(a, sp),
      physics_compiler(a, sp),
      actor_compiler(a, sp),
      shape_compiler(a, sp),
      sprite_compiler(a, sp),
      texture_compiler(a, sp),
      unit_compiler(a, sp),
      audio_compiler(a, sp),
      sound_compiler(a, sp),
      particle_compiler(a, sp),
      file_watcher(a),
      file_watch_events(a) {};
    IdLookupTable<Project*>  projects;
    Allocator        *allocator;
    Json             *compilers_config;
    StringPool        sp;
    Array<Work>       work_buf;
    FileWatcher       file_watcher;
    Array<WatchEvent> file_watch_events;
    FontCompiler      font_compiler;
    ShaderCompiler    shader_compiler;
    AnimsetCompiler   animset_compiler;
    LevelCompiler     level_compiler;
    PackageCompiler   package_compiler;
    PhysicsCompiler   physics_compiler;
    ActorCompiler     actor_compiler;
    ShapeCompiler     shape_compiler;
    SpriteCompiler    sprite_compiler;
    ScriptCompiler    script_compiler;
    TextureCompiler   texture_compiler;
    UnitCompiler      unit_compiler;
    AudioCompiler     audio_compiler;
    SoundCompiler     sound_compiler;
    ParticleCompiler  particle_compiler;
  };

  CompileContext *compile_ctx    = NULL;


  ResourceType fname_to_type(const char *fname)
  {
    const char *file_ext = strrchr(fname, '.');

    if (!file_ext) return RESOURCE_TYPE_DATA;

    file_ext++;
    for (int i = 1; i < NumResourceExtension; i++) {
      if (strcmp(file_ext, ResourceExtension[i]) == 0)
        return (ResourceType)i;
    }
    return RESOURCE_TYPE_DATA;
  }

  void fname_to_rname(const char *fname, char *rname)
  {
    // if the type is undefined, keep the extension in the name
    const u32 type = fname_to_type(fname);
    const char *fname_end =
      (type == RESOURCE_TYPE_DATA) ? fname + strlen(fname) : strrchr(fname, '.');
    char *c = (char*)fname;

    // replace '\' by '/'
    while (c < fname_end) {
      *rname++ = (*c == '\\') ? '/' : *c;
      ++c;
    }
    *rname = '\0';
  }

  void save_data(const Hash<Metadata> &metadata, const char *path)
  {
    FILE *file = fopen(path, "wb");

    //save metadata
    const Hash<Metadata>::Entry *m    = hash::begin(metadata),
      *mend = hash::end(metadata);
    // write the number of metadata
    u32 sz = hash::size(metadata);
    fwrite((void*)&sz, sizeof(u32), 1, file);
    // write metadata
    for (; m < mend; m++) {
      fwrite(&m->key, sizeof(u64), 1, file);
      fwrite(&m->value, sizeof(Metadata), 1, file);
    }
    fflush(file);
    fclose(file);
  }

  Json *get_json(const char *path, Allocator &a, StringPool &sp)
  {
    if (file_exists(path)) {
      Json *json = MAKE_NEW(a, Json, a, sp);
      if (json::parse_from_file(*json, json::root(*json), path))
        return json;

      MAKE_DELETE(a, Json, json);
    }
    XERROR("File not found : \"%s\"", path);
    return NULL;
  }

  void load_data(Hash<Metadata> &metadata, const char *path)
  {
    FILE *file = fopen(path, "rb");
    u32   num_items;

    if (file == NULL) return;

    { // load metadata
      char item_buf[PGC_MTD_SIZE];

      // read & reserve the number of items
      fread((void*)&num_items, sizeof(u32), 1, file);
      hash::reserve(metadata, num_items);

      // populate the metadata hash
      while (fread((void*)item_buf, PGC_MTD_SIZE, 1, file)) {
        hash::set(metadata, (*(u64*)item_buf), *((Metadata*)(item_buf + 8)));
      }
    }
    fclose(file);
  }

  void delete_resource(Project &project, u64 id)
  {
    Work w;
    w.project = &project;
    w.id.as64 = id;
    w.state   = STATE_DELETED;
    w.src     = NULL;

    hash::remove(project.metadata, w.id.as64);

    array::push_back(compile_ctx->work_buf, w);
  }

  u64 create_or_update_resource(Project &project, const char *dir, const WIN32_FIND_DATA &fd)
  {
    char path[MAX_PATH];
    char name[MAX_PATH];

    Work     w;
    Metadata mtd;

    sprintf(path, "%s\\%s", dir, fd.cFileName);

    fname_to_rname(path + strlen(project.src_dir) + 1, name);
    w.id.fields.type = (u32)fname_to_type(fd.cFileName);
    w.id.fields.name = id_string_manager::create(project.id_string_ctx, name);

    mtd.size  = (fd.nFileSizeHigh * ((u64)MAXDWORD + 1)) + fd.nFileSizeLow;
    mtd.mtime = ((u64)fd.ftLastWriteTime.dwLowDateTime
                 | ((u64)fd.ftLastWriteTime.dwHighDateTime) << 32);

    // if the file already have metadata
    if (hash::has(project.metadata, w.id.as64)) {
      Metadata def;
      const Metadata &old_mtd = hash::get(project.metadata, w.id.as64, def);
      // if the resource has not changed since last compile, return.
      if (mtd.size == old_mtd.size && mtd.mtime == old_mtd.mtime)
        return w.id.as64;
      else
        w.state = STATE_UPDATED;
    } else {
      w.state = STATE_CREATED;
    }

    // update the metadata
    hash::set(project.metadata, w.id.as64, mtd);
    w.src      = string_pool::acquire(compile_ctx->sp, path);
    w.project  = &project;

    array::push_back(compile_ctx->work_buf, w);

    return w.id.as64;
  }

  void scan_source(Project &project, const char *dir, Hash<char> &scanned)
  {
    WIN32_FIND_DATA fdFile;
    HANDLE hFind = NULL;

    char path[MAX_PATH];

    // specify a file mask
    sprintf(path, "%s\\*.*", dir);

    hFind = FindFirstFile(path, &fdFile);
    do {
      XASSERT(hFind != INVALID_HANDLE_VALUE, "Path not found: [%s]\n", path);

      // find first file will always return "."
      // and ".." as the first two directories.
      if (strcmp(fdFile.cFileName, ".") != 0
          && strcmp(fdFile.cFileName, "..") != 0) {
        sprintf(path, "%s\\%s", dir, fdFile.cFileName);
        // is it a folder?
        if (fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          scan_source(project, path, scanned);
        else
          hash::set(scanned, create_or_update_resource(project, dir, fdFile), 'o');
      }
    } while (FindNextFile(hFind, &fdFile));
    FindClose(hFind);
  }

  inline CompilerBase *get_compiler(Work &w)
  {
    switch ((ResourceType)w.id.fields.type) {
    case RESOURCE_TYPE_FONT:
      return (CompilerBase*)&compile_ctx->font_compiler;
    case RESOURCE_TYPE_SHADER:
      return (CompilerBase*)&compile_ctx->shader_compiler;
    case RESOURCE_TYPE_ANIMSET:
      return (CompilerBase*)&compile_ctx->animset_compiler;
    case RESOURCE_TYPE_LEVEL:
      return (CompilerBase*)&compile_ctx->level_compiler;
    case RESOURCE_TYPE_PACKAGE:
      return (CompilerBase*)&compile_ctx->package_compiler;
    case RESOURCE_TYPE_PHYSICS:
      return (CompilerBase*)&compile_ctx->physics_compiler;
    case RESOURCE_TYPE_ACTOR:
      return (CompilerBase*)&compile_ctx->actor_compiler;
    case RESOURCE_TYPE_SHAPE:
      return (CompilerBase*)&compile_ctx->shape_compiler;
    case RESOURCE_TYPE_SPRITE:
      return (CompilerBase*)&compile_ctx->sprite_compiler;
    case RESOURCE_TYPE_SCRIPT:
      return (CompilerBase*)&compile_ctx->script_compiler;
    case RESOURCE_TYPE_TEXTURE:
      return (CompilerBase*)&compile_ctx->texture_compiler;
    case RESOURCE_TYPE_UNIT:
      return (CompilerBase*)&compile_ctx->unit_compiler;
    case RESOURCE_TYPE_AUDIO:
      return (CompilerBase*)&compile_ctx->audio_compiler;
    case RESOURCE_TYPE_SOUND:
      return (CompilerBase*)&compile_ctx->sound_compiler;
    case RESOURCE_TYPE_PARTICLE:
      return (CompilerBase*)&compile_ctx->particle_compiler;
    default:
      LOG("Compiler not found, type: %d, project %s", w.id.fields.type, w.project->data_dir);
      return NULL;
    }
  }

  void do_work(void)
  {
    Work *wend = array::end(compile_ctx->work_buf);
    Allocator &a = memory_globals::default_allocator();

    if (array::size(compile_ctx->work_buf) == 0)
      return;

    // -------------------------------------------------------------------------
    // Add dependent resources
    // -------------------------------------------------------------------------
    {
      char dep_path[MAX_PATH];

      TempAllocator512 ta(a);
      Array<u64> dependents(ta);
      for (Work *w = array::begin(compile_ctx->work_buf); w < wend; ++w) {
        if (w->state != STATE_UPDATED)
          continue;

        linkage_manager::get_dependents(w->project->linkage_ctx, w->id.as64, dependents, true);
        for (u32 i = 0; i < array::size(dependents); i++) {
          Work dw;
          dw.id.as64  = dependents[i];
          dw.state    = STATE_UPDATED;
          dw.project  = w->project;

          if (dw.id.fields.type == RESOURCE_TYPE_DATA)
            sprintf(dep_path, "%s/%s", w->project->src_dir,
            id_string_manager::lookup(w->project->id_string_ctx, dw.id.fields.name));
          else
            sprintf(dep_path, "%s/%s.%s", w->project->src_dir,
            id_string_manager::lookup(w->project->id_string_ctx, dw.id.fields.name),
            ResourceExtension[dw.id.fields.type]);

          dw.src = string_pool::acquire(compile_ctx->sp, dep_path);
          array::push_back(compile_ctx->work_buf, dw);
        }
      }
    }

    // -------------------------------------------------------------------------
    // Remove duplicates
    // -------------------------------------------------------------------------

    {
      for (u32 i = 0; i < array::size(compile_ctx->work_buf); i++) {
        for (u32 j = i + 1; j < array::size(compile_ctx->work_buf); j++) {
          while (compile_ctx->work_buf[i].id.as64 == compile_ctx->work_buf[j].id.as64
                 && j < array::size(compile_ctx->work_buf)) {

            if (compile_ctx->work_buf[j].state == STATE_DELETED)
              compile_ctx->work_buf[i] = compile_ctx->work_buf[j];

            string_pool::release(compile_ctx->sp, compile_ctx->work_buf[j].src);
            compile_ctx->work_buf[j] = array::pop_back(compile_ctx->work_buf);
          }
        }
      }
    }

    // -------------------------------------------------------------------------
    // Consume Buffer
    // -------------------------------------------------------------------------

    wend = array::end(compile_ctx->work_buf);
    for (Work *w = array::begin(compile_ctx->work_buf); w < wend; ++w) {

      linkage_manager::remove_all(w->project->linkage_ctx, w->id.as64);

      if (w->state == STATE_DELETED) { // if the resource is tagged as missing, and delete its data
        data_manager::remove(w->project->data_ctx, w->id);
        OUTPUT("[Compiler] %s %s deleted",
               ResourceTypeNames[w->id.fields.type],
               id_string_manager::lookup(w->project->id_string_ctx, w->id.fields.name));
      }

      if ((w->state == STATE_CREATED || w->state == STATE_UPDATED)
          && w->id.fields.type != RESOURCE_TYPE_DATA) {
        w->data = data_manager::open_write(w->project->data_ctx, w->id);

        bool result = true;
        Timer compile_timer;
        start_timer(compile_timer);

        CompilerBase *compiler = get_compiler(*w);

        u32 memory_part_size = 0;
        u32 stream_part_size = 0;

        if (compiler) {
          result = compiler->compile(*w);
          fseek(w->data, 0, SEEK_END);
          memory_part_size = ftell(w->data);
          if (result && compiler->has_stream()) {
            result = compiler->compile_stream(*w);
            fseek(w->data, 0, SEEK_END);
            stream_part_size = ftell(w->data) - memory_part_size;
          }
        }

        data_manager::close_write(w->project->data_ctx, w->id, w->data, memory_part_size, stream_part_size);

        if (result) {
          stop_timer(compile_timer);
          OUTPUT("[Compiler] %s : \"%s\", compiled in %.2f ms",
                 ResourceTypeNames[w->id.fields.type],
                 id_string_manager::lookup(w->project->id_string_ctx, w->id.fields.name),
                 get_elapsed_time_in_ms(compile_timer));
          w->state = STATE_COMPILE_SUCCESS;
        }
        else {
          HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
          //sets the color to intense red on blue background
          SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
          OUTPUT("[Compiler] error compiling \"%s.%s\"",
                 id_string_manager::lookup(w->project->id_string_ctx, w->id.fields.name),
                 ResourceExtension[w->id.fields.type]);
          //reverting back to the normal color
          SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

          data_manager::remove(w->project->data_ctx, w->id);
          hash::remove(w->project->metadata, w->id.as64);
          w->state = STATE_COMPILE_ERROR;
        }
      }

      // -------------------------------------------------------------------------
      // TMP : clean & update
      // -------------------------------------------------------------------------
      if (w->src)
        string_pool::release(compile_ctx->sp, w->src);
    }

    const IdLookupTable<Project*>::Entry *p = idlut::begin(compile_ctx->projects),
      *pend = idlut::end(compile_ctx->projects);
    for (; p < pend; p++) {
      data_manager::save(p->value->data_ctx);
      linkage_manager::save(p->value->linkage_ctx);
      id_string_manager::save(p->value->id_string_ctx);
    }

    array::clear(compile_ctx->work_buf);
  }

  /*
  class SourceUpdateListener : public FW::FileWatchListener
  {
  private:
    Project *_project;
  public:
    SourceUpdateListener(Project &project) : _project(&project) {}
    void handleFileAction(FW::WatchID watchid, const std::string &dir, const std::string &filename, FW::Action action)
    {
      (void)watchid;
      WIN32_FIND_DATA fdFile;
      HANDLE hFind = NULL;
      char path[MAX_PATH];
      const char *file_name = filename.c_str();
      snprintf(path, MAX_PATH, "%s\\%s", dir.c_str(), file_name);

      switch (action) {
        case efsw::Actions::Add:
        case efsw::Actions::Modified:
        if (strcmp(file_name, SETTINGS_FILE_NAME) == 0 &&
            strcmp(_project->src_dir, dir.c_str()) == 0) {
          char dest[MAX_PATH];
          sprintf(dest, "%s\\%s", _project->data_dir, filename);
          copy_file(dest, path);
          OUTPUT("[Compiler] \"%s\" copied", SETTINGS_FILE_NAME);
          return;
        }
        hFind = FindFirstFile(path, &fdFile);
        path[strlen(path) - strlen(fdFile.cFileName) - 1] = '\0';
        create_or_update_resource(*_project, path, fdFile);
        FindClose(hFind);
        break;
        case efsw::Actions::Delete:
        if (strcmp(file_name, SETTINGS_FILE_NAME) == 0 &&
            strcmp(_project->src_dir, dir.c_str()) == 0) {
          char dest[MAX_PATH];
          sprintf(dest, "%s\\%s", _project->data_dir, filename);
          delete_file(dest);
          OUTPUT("[Compiler] \"%s\" removed", SETTINGS_FILE_NAME);
          return;
        }
        DataId id;
        fname_to_rname(path + strlen(_project->src_dir) + 1, path);
        id.fields.type = fname_to_type(file_name);
        id.fields.name = murmur_hash_32(path);
        delete_resource(*_project, id.as64);
        break;
      }
    }
  };
  */

  void shutdown_project(Project *p)
  {
    // save metadatas
    char szData[MAX_PATH];
    sprintf(szData, "%s\\%s", p->data_dir, "build.dat");
    save_data(p->metadata, szData);

    // clean project
    if (p->watched) stop_watch(compile_ctx->file_watcher, p->file_listener);
    
    data_manager::clean(p->data_ctx);
    data_manager::shutdown(p->data_ctx);
    linkage_manager::shutdown(p->linkage_ctx);
    id_string_manager::shutdown(p->id_string_ctx);
    sqlite3_check(sqlite3_close(p->db));

    // removes config files
    if (p->compilers_config && p->compilers_config != compile_ctx->compilers_config)
      MAKE_DELETE((*compile_ctx->allocator), Json, p->compilers_config);

    MAKE_DELETE((*compile_ctx->allocator), Json, p->settings_config);

    MAKE_DELETE((*compile_ctx->allocator), Project, p);
  }

  // creates the build database if necessary and opens it
  void create_and_open_build_db(sqlite3 **db, const char *szdir) {
    char szdb[MAX_PATH];
    bool created = false;
    HANDLE h;

    sprintf(szdb, "%s\\%s", szdir, "build.db");

    if (GetFileAttributes(szdb) == 0xFFFFFFFF) {
      h = CreateFile(szdb, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
      XASSERT(h, "could not create the build database database \"%s\".", szdb);
      CloseHandle(h);
      created = true;
    }

    sqlite3_check(sqlite3_open(szdb, db));

    if (created) {
      // create the resource types table
      TempAllocator1024 ta;
      Buffer sqlbuf(ta);

      sqlbuf <<
        "CREATE TABLE types ("
        "  id   INT,"
        "  name TEXT,"
        "  ext  TEXT,"
        "  PRIMARY KEY (id)"
        ");";

      sqlbuf << "INSERT INTO types (id, name, ext) VALUES ";

      const char sql_value[] = "(%d, '%s', '%s'),";
      char tmp_str[BUFSIZ];
      for (int i = 0; i < NumResourceExtension; i++) {
        sprintf(tmp_str, sql_value, i, ResourceTypeNames[i], ResourceExtension[i]);
        push(sqlbuf, tmp_str, strlen(tmp_str));
      }

      array::resize(sqlbuf, array::size(sqlbuf) - 1);
      sqlbuf << ";";
      sqlite3_check(sqlite3_exec(*db, c_str(sqlbuf), 0, 0, 0));
    }
  }
}

namespace pge
{
  namespace compile_manager
  {
    bool init(const char *config_folder, const char *schemas_folder, Allocator &a)
    {
      char config_path[MAX_PATH];
      strcpy(config_path, config_folder);

      if (config_path[strlen(config_path) - 1] != '\\')
        strcat(config_path, "\\");
      strcat(config_path, SERVER_COMPILE_FILE_NAME);

      compile_ctx = MAKE_NEW(a, CompileContext, a);

      compile_ctx->compilers_config = get_json(config_path, a, compile_ctx->sp);

      if (!compile_ctx->compilers_config) {
        char out[BUFSIZ];
        sprintf(out, "The file system \"%s\" is missing.\n", SERVER_COMPILE_FILE_NAME);
        printf(out);
        LOG(out);
        return false;
      }
      
      if (!load_schemas(schemas_folder, a, compile_ctx->sp)) return false;

      return validate_compile_config(*compile_ctx->compilers_config);
    }

    void start_watching(u64 id)
    {
      if (!idlut::has(compile_ctx->projects, id))
        return;

      Project *p = *idlut::lookup(compile_ctx->projects, id);
      p->file_listener = start_watch(compile_ctx->file_watcher, p->src_dir, true);
      p->watched = true;
    }

    void stop_watching(u64 id)
    {
      if (!idlut::has(compile_ctx->projects, id)) return;
      Project *p = *idlut::lookup(compile_ctx->projects, id);
      if (!p->watched) return;
      stop_watch(compile_ctx->file_watcher, p->file_listener);
      p->watched = false;
    }

    u64 add(const char *src, const char *data)
    {
      Project *project = MAKE_NEW(*compile_ctx->allocator, Project, *compile_ctx->allocator, compile_ctx->sp);
      strcpy(project->src_dir, src);
      strcpy(project->data_dir, data);

      u64 id = idlut::add(compile_ctx->projects, project);

      _fullpath(project->src_dir, project->src_dir, MAX_PATH);
      _fullpath(project->data_dir, project->data_dir, MAX_PATH);

      if (!directory_exists(project->src_dir)) {
        char out[BUFSIZ];
        sprintf(out, "Could not find the source directory \"%s\".\n", project->src_dir);
        printf(out);
        LOG(out);
        remove(id);
        return id;
      }

      if (!directory_exists(project->data_dir)) {
        if (!CreateDirectory(project->data_dir, NULL)) {
          char out[BUFSIZ];
          sprintf(out, "Could not create the data directory at \"%s\".\n", project->data_dir);
          printf(out);
          LOG(out);
          remove(id);
          return id;
        }
      }

      // create the build database and startup data & dependency managers
      create_and_open_build_db(&project->db, project->data_dir);
      data_manager::startup(project->data_ctx, project->data_dir, project->db);
      linkage_manager::startup(project->linkage_ctx, project->db);
      id_string_manager::startup(project->id_string_ctx, project->db);
      
      { // Loads metadata from build.dat
        char szData[MAX_PATH];
        sprintf(szData, "%s\\%s", project->data_dir, "build.dat");
        load_data(project->metadata, szData);
      }

      char path[MAX_PATH];

      // loads compilers config
      sprintf(path, "%s\\%s", project->src_dir, COMPILE_FILE_NAME);
      project->compilers_config = get_json(path, *compile_ctx->allocator, compile_ctx->sp);
      if (project->compilers_config) {
        json::merge(*project->compilers_config, *compile_ctx->compilers_config,
                    json::root(*project->compilers_config), json::root(*compile_ctx->compilers_config), false);
        if (!validate_project_compilers_config(*project->compilers_config, project->src_dir)) {
          remove(id);
          return id;
        }
      } else {
        project->compilers_config = compile_ctx->compilers_config;
      }

      // loads settings config
      sprintf(path, "%s\\%s", project->src_dir, SETTINGS_FILE_NAME);
      project->settings_config = get_json(path, *compile_ctx->allocator, compile_ctx->sp);
      if (!validate_project_settings_config(*project->settings_config, project->src_dir)) {
        remove(id);
        return id;
      }
      return id;
    }

    bool project_loaded(u64 id)
    {
      return compile_ctx != NULL && idlut::has(compile_ctx->projects, id);
    }

    void remove(u64 id)
    {
      if (compile_ctx == NULL || !idlut::has(compile_ctx->projects, id)) return;

      shutdown_project(*idlut::lookup(compile_ctx->projects, id));
      idlut::remove(compile_ctx->projects, id);
    }

    bool build(u64 id)
    {
      if (compile_ctx == NULL || !idlut::has(compile_ctx->projects, id)) return false;

      Project *p = *idlut::lookup(compile_ctx->projects, id);
      {
        TempAllocator4096 ta;
        Hash<char> scanned(ta);
        scan_source(*p, p->src_dir, scanned);

        const Hash<Metadata>::Entry *m    = hash::begin(p->metadata);
        const Hash<Metadata>::Entry *mend = hash::end(p->metadata);
        // write metadata
        for (; m < mend; m++) {
          if (!hash::has(scanned, m->key))
            delete_resource(*p, m->key);
        }
        do_work();
      }

      char src[MAX_PATH];
      sprintf(src, "%s\\%s", p->src_dir, SETTINGS_FILE_NAME);
      if (file_exists(src)) {
        char dest[MAX_PATH];
        sprintf(dest, "%s\\%s", p->data_dir, SETTINGS_FILE_NAME);

        copy_file(dest, src);
        OUTPUT("[Compiler] \"%s\" copied", SETTINGS_FILE_NAME);
      }

      p->first_scan = false;

      return true;
    }

    bool rebuild(u64 id)
    {
      if (compile_ctx == NULL || !idlut::has(compile_ctx->projects, id)) return false;

      Project *p = *idlut::lookup(compile_ctx->projects, id);

      bool watched = p->watched;
      if (watched)
        stop_watching(id);

      data_manager::shutdown(p->data_ctx);
      linkage_manager::shutdown(p->linkage_ctx);
      id_string_manager::shutdown(p->id_string_ctx);
      sqlite3_check(sqlite3_close(p->db));


      int err_code = delete_directory_content(p->data_dir);
      XASSERT(!err_code, "Cannot remove data directory : err %d", err_code);

      hash::clear(p->metadata);

      create_and_open_build_db(&p->db, p->data_dir);
      data_manager::startup(p->data_ctx, p->data_dir, p->db);
      linkage_manager::startup(p->linkage_ctx, p->db);
      id_string_manager::startup(p->id_string_ctx, p->db);

      if (watched)
        start_watching(id);

      return build(id);
    }

    // TODO : bundle
    void bundle(u64 id)
    {
      if (compile_ctx == NULL || !idlut::has(compile_ctx->projects, id)) return;
    }

    void update(void)
    {
      if (idlut::size(compile_ctx->projects))
      {
        Project *p = idlut::begin(compile_ctx->projects)->value;
        if (p->file_listener)
        {
          Array<WatchEvent> &events = compile_ctx->file_watch_events;
          get_watch_events(compile_ctx->file_watcher, p->file_listener, events);
          if (array::size(events))
          {
            WatchEvent *e = array::begin(events),
              *eend = array::end(events);

            // TODO: consommer le buff d'event
            for (; e < eend; e++){
              OUTPUT("COMPILE MANAGER\n\tACTION type: %d", e->type);
             // OUTPUT("ACTION type: %d\n\tdir: %s\n\tfilename: %s", e->type, e->dir, e->filename);
            }
        
            array::clear(events);
          }
        }
      }

      do_work();
    }

    void shutdown(void)
    {
      unload_schemas();

      // Cleans project stuff
      const IdLookupTable<Project*>::Entry *e = idlut::begin(compile_ctx->projects),
        *end = idlut::end(compile_ctx->projects);
      for (; e < end; e++)
        shutdown_project(e->value);

      // Cleans environement singleton
      MAKE_DELETE((*compile_ctx->allocator), Json, compile_ctx->compilers_config);
      MAKE_DELETE((*compile_ctx->allocator), CompileContext, compile_ctx);
    }
  }
}