/*
 * Codec2RoundTrip - encode and decode one frame per mode, on device.
 *
 * Doubles as the library's embedded link test: it touches create, encode,
 * decode, the frame-geometry accessors and destroy, so anything the weeding
 * broke shows up as a link error rather than a runtime surprise months later.
 * See examples/platformio.ini.
 *
 * Prints a table over Serial and the per-mode encode time, which is the number
 * that decides whether a target can carry voice in real time at all.
 */
#include <codec2.h>

struct ModeInfo {
    int mode;
    const char *name;
};

static const ModeInfo MODES[] = {
    {CODEC2_MODE_2400, "2400"},
    {CODEC2_MODE_1400, "1400"},
    {CODEC2_MODE_700C, "700C"},
    {CODEC2_MODE_450, "450"},
};

// Frames are at most 640 samples (450PWB decodes at 16 kHz); the modes above
// are all 320 or less. Static buffers keep this off the heap so the example
// runs on parts where 700C's own allocation is already the tight bit.
static int16_t speech[640];
static int16_t decoded[640];
static uint8_t bits[16];

static void runMode(const ModeInfo &m)
{
    struct CODEC2 *c2 = codec2_create(m.mode);
    if (!c2) {
        Serial.printf("%-5s  not compiled into this build\n", m.name);
        return;
    }

    const int nsam = codec2_samples_per_frame(c2);
    const int nbyte = codec2_bytes_per_frame(c2);

    // A 400 Hz square wave. Silence is special-cased by several modes, so it
    // would not prove the encoder ran.
    for (int n = 0; n < nsam; n++)
        speech[n] = (n % 20) < 10 ? 8000 : -8000;

    // Average over a few frames - a single one is inside the timer's noise on
    // faster parts, and the encoder carries state between frames anyway.
    const int reps = 10;
    const uint32_t t0 = micros();
    for (int i = 0; i < reps; i++)
        codec2_encode(c2, bits, speech);
    const uint32_t encUs = (micros() - t0) / reps;

    const uint32_t t1 = micros();
    for (int i = 0; i < reps; i++)
        codec2_decode(c2, decoded, bits);
    const uint32_t decUs = (micros() - t1) / reps;

    // Frame duration in microseconds, so the caller can see the real-time
    // margin without doing the arithmetic.
    const uint32_t frameUs = (uint32_t)nsam * 1000000UL / 8000UL;

    Serial.printf("%-5s  %3d samp  %2d B  enc %5lu us  dec %5lu us  frame %5lu us  load %2lu%%\n", m.name, nsam, nbyte,
                  (unsigned long)encUs, (unsigned long)decUs, (unsigned long)frameUs,
                  (unsigned long)((encUs + decUs) * 100 / frameUs));

    codec2_destroy(c2);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial && millis() < 5000) {
    }

    Serial.println();
    Serial.println("codec2 round trip");
    Serial.println("mode   frame     size  encode      decode      budget      cpu");
    Serial.println("--------------------------------------------------------------");

    for (const ModeInfo &m : MODES)
        runMode(m);

    Serial.println();
    Serial.println("load over 100% means this part cannot keep up in real time.");
}

void loop()
{
    delay(1000);
}
