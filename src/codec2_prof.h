/* Optional cycle profiling for the encode/decode internals.

   The library cannot read a cycle counter portably, so the application
   supplies one: point codec2_prof_clock at something like the Cortex-M DWT
   CYCCNT reader and the counters below fill in. Leave it NULL and every macro
   compiles to nothing measurable.

   Build with -DCODEC2_PROFILE to enable.
*/

#ifndef __CODEC2_PROF__
#define __CODEC2_PROF__

#include <stdint.h>

#ifdef CODEC2_PROFILE

#define C2PROF_SLOTS 8

#ifdef __cplusplus
extern "C" {
#endif
extern uint32_t (*codec2_prof_clock)(void);
extern uint32_t codec2_prof_cyc[C2PROF_SLOTS];
extern uint32_t codec2_prof_cnt[C2PROF_SLOTS];
#ifdef __cplusplus
}
#endif

#define C2PROF_BEGIN(slot)                                                                                             \
    uint32_t c2prof_t##slot = codec2_prof_clock ? codec2_prof_clock() : 0

#define C2PROF_END(slot)                                                                                               \
    do {                                                                                                               \
        if (codec2_prof_clock) {                                                                                       \
            codec2_prof_cyc[slot] += codec2_prof_clock() - c2prof_t##slot;                                             \
            codec2_prof_cnt[slot]++;                                                                                   \
        }                                                                                                              \
    } while (0)

#else
#define C2PROF_BEGIN(slot)
#define C2PROF_END(slot)
#endif

#endif
