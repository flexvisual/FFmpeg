/* Flexvisual Hap R BC7 compatibility-test encoder; LGPL-2.1-or-later. */
#ifndef AVCODEC_HAPBC7ENC_H
#define AVCODEC_HAPBC7ENC_H
#include <stddef.h>
#include <stdint.h>
/* Encode one 4x4 RGBA8 block as a 16-byte BC7 mode 6 block. */
int ff_hap_bc7_encode_block(uint8_t *dst, ptrdiff_t stride, const uint8_t *src);
#endif
