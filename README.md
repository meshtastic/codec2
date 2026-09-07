# codec2

[Codec 2](http://www.rowetel.com/codec2.html) by David Rowe, packaged as an Arduino / PlatformIO
library carrying **the vocoder only**.

Upstream `libcodec2` ships an HF modem stack, LDPC and Golay FEC, and the FreeDV API alongside the
speech codec. None of that is used when the transport is a LoRa mesh, so it is not here: 104 of the
original 158 source files are gone and `src/` drops from 4.1 MB to 1.0 MB.

## Modes

| Mode | Bitrate | Frame | Bits/frame | Bytes/frame |
|---|---|---|---|---|
| `CODEC2_MODE_3200` | 3200 | 20 ms | 64 | 8 |
| `CODEC2_MODE_2400` | 2400 | 20 ms | 48 | 6 |
| `CODEC2_MODE_1600` | 1600 | 40 ms | 64 | 8 |
| `CODEC2_MODE_1400` | 1400 | 40 ms | 56 | 7 |
| `CODEC2_MODE_1300` | 1300 | 40 ms | 52 | 7 |
| `CODEC2_MODE_1200` | 1200 | 40 ms | 48 | 6 |
| `CODEC2_MODE_700C` | 700 | 40 ms | 28 | 4 |
| `CODEC2_MODE_450` | 450 | 40 ms | 18 | 3 |
| `CODEC2_MODE_450PWB` | 450 | 40 ms | 18 | 3 |

`450PWB` is a **decoder variant of 450**, not a separate bitrate. The bitstream is identical and the
encoder is the same function; only the decoder differs, rendering at 16 kHz (640 samples/frame)
instead of 8 kHz. Creating a `450PWB` instance for encoding is undefined behaviour, because its
constants are built at 16 kHz. Encode with `450` and decode with `450PWB` if you want the wideband
rendering.

Legacy modes `700` and `700B` do not exist. They were removed upstream and are not coming back.

## Usage

```c
#include <codec2.h>

struct CODEC2 *c2 = codec2_create(CODEC2_MODE_700C);   // NULL if the mode is compiled out
int nsam  = codec2_samples_per_frame(c2);              // 320
int nbyte = codec2_bytes_per_frame(c2);                // 4

short         speech[nsam];
unsigned char bits[nbyte];

codec2_encode(c2, bits, speech);
codec2_decode(c2, speech, bits);

codec2_destroy(c2);
```

Always check `codec2_create()` for `NULL`. It returns null both for an invalid mode id and for a mode
that was compiled out.

## Compiling out modes

Each mode's codebooks cost flash. Build only what you ship:

```ini
build_flags =
  -DCODEC2_MODE_EN_DEFAULT=0
  -DCODEC2_MODE_2400_EN=1
  -DCODEC2_MODE_1400_EN=1
  -DCODEC2_MODE_700C_EN=1
  -DCODEC2_MODE_450_EN=1
```

Approximate table cost per mode. `lsp_cb` and `ge_cb` are shared, so the classic LSP modes are nearly
free together; the low-bitrate end is where the flash goes:

| Mode | Tables | Flash |
|---|---|---|
| 3200 / 1600 / 1300 | `lsp_cb` | 0.7 KB |
| 2400 / 1400 | `lsp_cb` + `ge_cb` | 2.7 KB |
| 1200 | `lsp_cbjmv` + `ge_cb` | 43 KB |
| 700C | `newamp1vq_cb` | 82 KB |
| 450 / 450PWB | `newamp2vq_cb` | 82 KB |

The mode switches gate runtime dispatch; the disabled modes' functions are still compiled. Dropping
their code and codebooks relies on `-ffunction-sections -fdata-sections -Wl,--gc-sections`, which the
`espressif32` and `nordicnrf52` PlatformIO builders enable by default. Verify with a real size report
on your target rather than trusting the table above.

## Requirements

**A hardware FPU.** Codec 2 is floating point throughout - `quantise.c` alone makes 15 `log10f` and 11
`powf` calls per frame. Parts without one (ESP32-S2, ESP32-C3, ESP32-C6, RP2040, Cortex-M0/M0+) cannot
run it in real time: measurements on RP2040 put software float at 3.54 s of CPU per second of audio.

Known good: ESP32, ESP32-S3, ESP32-P4, nRF52840 (M4F), RP2350 (M33), STM32F4.

## Provenance

Forked from [sh123/esp32_codec2_arduino](https://github.com/sh123/esp32_codec2_arduino), which tracks
the stable branch of [drowe67/codec2-dev](https://github.com/drowe67/codec2-dev). Upstream `main` is
moving toward RADE and dropping the low bit rate modes, so the stable line is the one to follow.

This repository starts from a single commit rather than carrying that history. The vocoder sources
are unmodified apart from the mode switches and the external codebook support, and every file keeps
its original copyright and licence header.

## License

LGPL-2.1-or-later, as upstream: the vocoder sources carry David Rowe's LGPL 2.1 headers and
`LICENSE` is the matching licence text.
