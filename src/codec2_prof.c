/* Storage for the optional profiling counters. See codec2_prof.h. */

#include "codec2_prof.h"

#ifdef CODEC2_PROFILE
uint32_t (*codec2_prof_clock)(void) = 0;
uint32_t codec2_prof_cyc[C2PROF_SLOTS];
uint32_t codec2_prof_cnt[C2PROF_SLOTS];
#endif
