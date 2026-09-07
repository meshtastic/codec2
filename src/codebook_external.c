/* Runtime binding for CODEC2_EXTERNAL_CODEBOOKS. See codebook_q8.h. */

#include "codebook_q8.h"
#include "defines.h"

#ifdef CODEC2_EXTERNAL_CODEBOOKS

int codec2_set_external_codebooks(const int8_t *newamp1_stage1, const int8_t *newamp1_stage2, const int8_t *newamp2)
{
    if (!newamp1_stage1 || !newamp1_stage2 || !newamp2)
        return -1;

    newamp1vq_cb[0].cb = newamp1_stage1;
    newamp1vq_cb[1].cb = newamp1_stage2;
    newamp2vq_cb[0].cb = newamp2;
    return 0;
}

#endif
