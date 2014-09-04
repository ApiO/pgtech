#pragma once

#include <Windows.h>
#include <stdio.h>

#include "linkage_types.h"

namespace pge
{
  namespace linkage_manager
  {
    /// Starts a linkage manager.
    /// Data will be stored in the specified db.
    void startup          (LinkageManager &ctx, sqlite3 *db);

    /// Specifies that the resource named 'res_id' depends on the resource named 'dep_id'.
    void add_dependency   (LinkageManager &ctx, u64 res_id, u64 dep_id);

    /// Specifies that the resource named 'res_id' refers to the resource named 'ref_id'.
    void add_reference    (LinkageManager &ctx, u64 res_id, u64 ref_id);

    /// Returns the dependencies of the specified resource.
    void get_dependencies (const LinkageManager &ctx, u64 res_id, Array<u64> &dependencies, bool recursive = false);

    /// Returns the resources that depends on the specified resource.
    void get_dependents   (const LinkageManager &ctx, u64 res_id, Array<u64> &dependents,   bool recursive = false);

    /// Removes all the dependencies of a resource.
    void remove_all       (LinkageManager &ctx, u64 res_id);

    /// Saves the changes into the database.
    void save             (LinkageManager &ctx);

    /// Shuts down the system and frees its memory.
    void shutdown         (LinkageManager &ctx);
  }
}