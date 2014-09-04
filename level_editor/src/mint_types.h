#pragma once

#include <mintomic/mintomic.h>

#define BOOL(_mint32)(mint_load_32_relaxed(&(_mint32)) == 1u)
#define SET_MINT(_mint32, v)(mint_store_32_relaxed(&(_mint32), (v)))

 typedef mint_atomic32_t mint32;