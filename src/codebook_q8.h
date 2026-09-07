#ifndef __CODEBOOK_Q8__
#define __CODEBOOK_Q8__

#include <stdint.h>

/* The newamp1 and newamp2 VQ codebooks are mel-band amplitudes in dB, which
   span roughly -35..+56 dB and do not need float precision. Stored as int8 at
   half-dB steps they cost a quarter of the flash: 20.5 KB each rather than 82.

   Measured over every entry in both tables: RMS error 0.144 dB, worst case
   0.250 dB, nothing clipped. The VQ itself compresses 20 mel bands into 18
   bits, so its own quantisation error runs to many dB per band - this sits
   well under the codec's own noise floor. */

#define CODEBOOK_Q8_SCALE 0.5f

/* Dequantise one codebook entry. */
#define CODEBOOK_Q8(cb, i) ((float)(cb)[i] * CODEBOOK_Q8_SCALE)

/* With CODEC2_EXTERNAL_CODEBOOKS the tables are not linked into the image at
   all; the caller points them at memory-mapped storage before codec2_create().
   On nRF52840 that is external QSPI flash at 0x12000000, which is live as soon
   as nrfx_qspi_init() activates the peripheral. Saves ~41 KB of internal flash
   on a part where it is the scarce resource. */
#ifdef CODEC2_EXTERNAL_CODEBOOKS
#define CODEBOOK_CONST
#else
#define CODEBOOK_CONST const
#endif

struct lsp_codebook_q8 {
    int k;     /* dimension of each vector */
    int log2m; /* log2(m) */
    int m;     /* number of vectors */
    const int8_t *cb;
};

#ifdef CODEC2_EXTERNAL_CODEBOOKS
#ifdef __cplusplus
extern "C" {
#endif
/* Must be called before codec2_create() for any mode using 700C or 450.
   Returns 0 on success, -1 if any pointer is NULL. */
int codec2_set_external_codebooks(const int8_t *newamp1_stage1, const int8_t *newamp1_stage2, const int8_t *newamp2);
#ifdef __cplusplus
}
#endif
#endif

#endif
