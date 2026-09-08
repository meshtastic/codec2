/* Fast float powf for the LPC post filter.

   newlib-nano's powf costs about 2400 cycles on a Cortex-M4F - it works in
   double internally, so the f spelling saves nothing. lpc_post_filter calls it
   FFT_ENC/2 = 256 times per subframe, which measured 9.5 ms per subframe on an
   nRF52840 at 64 MHz, against a 10 ms budget.

   This is the usual log2/exp2 bit trick, about 20 cycles, with roughly 0.1%
   error. The post filter is perceptual weighting whose output is then energy
   normalised, so that error is far below anything audible.

   Opt in with CODEC2_FAST_MATH; without it the exact libm path is used.
*/

#ifndef __CODEC2_FASTPOW__
#define __CODEC2_FASTPOW__

#include <math.h>
#include <stdint.h>

static inline float codec2_fast_log2f(float x)
{
    union {
        float f;
        uint32_t i;
    } vx = {x};
    union {
        uint32_t i;
        float f;
    } mx = {(vx.i & 0x007FFFFFu) | 0x3f000000u};
    const float y = (float)vx.i * 1.1920928955078125e-7f;
    return y - 124.22551499f - 1.498030302f * mx.f - 1.72587999f / (0.3520887068f + mx.f);
}

static inline float codec2_fast_exp2f(float p)
{
    const float clipp = p < -126.0f ? -126.0f : p;
    const float z = clipp - (float)(int)clipp + (clipp < 0.0f ? 1.0f : 0.0f);
    union {
        uint32_t i;
        float f;
    } v = {(uint32_t)((1 << 23) * (clipp + 121.2740575f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z))};
    return v.f;
}

/* Matches powf for x > 0. Zero and negatives cannot reach the post filter -
   Rw[] is a magnitude spectrum - but log2 of zero is -inf, so guard anyway. */
static inline float codec2_fast_powf(float x, float p)
{
    if (x <= 0.0f)
        return 0.0f;
    return codec2_fast_exp2f(p * codec2_fast_log2f(x));
}

#ifdef CODEC2_FAST_MATH
#define CODEC2_POWF(x, p) codec2_fast_powf((x), (p))
#else
#define CODEC2_POWF(x, p) powf((x), (p))
#endif

#endif
