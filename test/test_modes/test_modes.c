/* Smoke test for the weeded library: every mode must create, report the frame
 * geometry the wire format depends on, and survive an encode/decode round trip.
 *
 *   pio test -e native
 *   pio test -e native-meshtastic-modes
 *
 * A mode compiled out via CODEC2_MODE_*_EN=0 is ignored rather than failed, so
 * the same suite validates both a full build and a trimmed one.
 */
#include <stdlib.h>
#include <unity.h>

#include "codec2.h"

struct expect {
    int mode;
    const char *name;
    int samples; /* per frame */
    int bits;    /* per frame */
    int encodes; /* 450PWB decodes only - encoding through it is undefined */
};

static const struct expect MODES[] = {
    {CODEC2_MODE_3200, "3200", 160, 64, 1},   {CODEC2_MODE_2400, "2400", 160, 48, 1},
    {CODEC2_MODE_1600, "1600", 320, 64, 1},   {CODEC2_MODE_1400, "1400", 320, 56, 1},
    {CODEC2_MODE_1300, "1300", 320, 52, 1},   {CODEC2_MODE_1200, "1200", 320, 48, 1},
    {CODEC2_MODE_700C, "700C", 320, 28, 1},   {CODEC2_MODE_450, "450", 320, 18, 1},
    {CODEC2_MODE_450PWB, "450PWB", 640, 18, 0},
};
#define NMODES (sizeof(MODES) / sizeof(MODES[0]))

void setUp(void) {}
void tearDown(void) {}

/* Frame geometry is wire-format visible - the audio module derives frames per
 * packet from it - so pin the numbers rather than just printing them. */
static void test_frame_geometry(void)
{
    for (size_t i = 0; i < NMODES; i++) {
        const struct expect *e = &MODES[i];
        struct CODEC2 *c2 = codec2_create(e->mode);
        if (!c2)
            continue; /* compiled out */

        TEST_ASSERT_EQUAL_INT_MESSAGE(e->samples, codec2_samples_per_frame(c2), e->name);
        TEST_ASSERT_EQUAL_INT_MESSAGE(e->bits, codec2_bits_per_frame(c2), e->name);
        TEST_ASSERT_EQUAL_INT_MESSAGE((e->bits + 7) / 8, codec2_bytes_per_frame(c2), e->name);

        codec2_destroy(c2);
    }
}

static void test_round_trip(void)
{
    for (size_t i = 0; i < NMODES; i++) {
        const struct expect *e = &MODES[i];
        if (!e->encodes)
            continue;

        struct CODEC2 *c2 = codec2_create(e->mode);
        if (!c2)
            continue;

        int nsam = codec2_samples_per_frame(c2);
        int nbyte = codec2_bytes_per_frame(c2);
        short *speech = calloc((size_t)nsam, sizeof(short));
        short *out = calloc((size_t)nsam, sizeof(short));
        unsigned char *bits = calloc((size_t)nbyte, 1);
        TEST_ASSERT_NOT_NULL(speech);
        TEST_ASSERT_NOT_NULL(out);
        TEST_ASSERT_NOT_NULL(bits);

        /* A 400 Hz square wave - voiced enough that the encoder has something to
         * model, unlike silence which several modes special-case. */
        for (int n = 0; n < nsam; n++)
            speech[n] = (short)((n % 20) < 10 ? 8000 : -8000);

        codec2_encode(c2, bits, speech);
        codec2_decode(c2, out, bits);

        int nonzero = 0;
        for (int n = 0; n < nsam; n++)
            if (out[n])
                nonzero++;
        TEST_ASSERT_GREATER_THAN_INT_MESSAGE(0, nonzero, e->name);

        free(speech);
        free(out);
        free(bits);
        codec2_destroy(c2);
    }
}

/* An unknown mode must return NULL, not a half-built state. The audio module
 * relies on this when a config names a mode the build compiled out. */
static void test_invalid_mode_returns_null(void)
{
    TEST_ASSERT_NULL(codec2_create(-1));
    TEST_ASSERT_NULL(codec2_create(99));
    TEST_ASSERT_NULL(codec2_create(6)); /* old CODEC2_MODE_700, removed upstream */
    TEST_ASSERT_NULL(codec2_create(7)); /* old CODEC2_MODE_700B, removed upstream */
}

/* At least one mode has to survive whatever the build enabled, or the switches
 * have silently produced a library that can do nothing. */
static void test_at_least_one_mode_available(void)
{
    int available = 0;
    for (size_t i = 0; i < NMODES; i++) {
        struct CODEC2 *c2 = codec2_create(MODES[i].mode);
        if (c2) {
            available++;
            codec2_destroy(c2);
        }
    }
    TEST_ASSERT_GREATER_THAN_INT(0, available);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_at_least_one_mode_available);
    RUN_TEST(test_frame_geometry);
    RUN_TEST(test_round_trip);
    RUN_TEST(test_invalid_mode_returns_null);
    return UNITY_END();
}
