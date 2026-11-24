/*
 * Minimal valid BC7 mode 6 encoder for Hap R compatibility-test media.
 * Copyright (C) 2026 Flexvisual contributors.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Diagnostic encoder, not production-quality rate/distortion optimization.
 * Mode 6: seven-bit one-hot mode prefix, two RGBA endpoints at seven bits
 * each plus endpoint p-bits, and four-bit pixel indices (3-bit anchor).
 */
#include "hapbc7enc.h"
#include <string.h>

static void put_bits(uint8_t *out, unsigned *bitpos, unsigned value, unsigned bits)
{
    unsigned b;
    for (b = 0; b < bits; ++b, ++*bitpos)
        out[*bitpos >> 3] |= ((value >> b) & 1u) << (*bitpos & 7);
}

int ff_hap_bc7_encode_block(uint8_t *dst, ptrdiff_t stride, const uint8_t *src)
{
    unsigned char pixels[16][4], endpoints[2][4], indices[16];
    int x, y, k, anchor, other, c, i;
    unsigned pos = 0;
    unsigned best_distance = 0;

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            const uint8_t *p = src + y * stride + x * 4;
            memcpy(pixels[y * 4 + x], p, 4);
        }
    }

    /* Approximate farthest-pair search in O(16). */
    anchor = 0;
    other = 0;
    for (k = 0; k < 16; k++) {
        unsigned distance = 0;
        for (c = 0; c < 4; c++) {
            int d = (int)pixels[0][c] - (int)pixels[k][c];
            distance += (unsigned)(d * d);
        }
        if (distance > best_distance) {
            best_distance = distance;
            anchor = k;
        }
    }
    best_distance = 0;
    for (k = 0; k < 16; k++) {
        unsigned distance = 0;
        for (c = 0; c < 4; c++) {
            int d = (int)pixels[anchor][c] - (int)pixels[k][c];
            distance += (unsigned)(d * d);
        }
        if (distance > best_distance) {
            best_distance = distance;
            other = k;
        }
    }
    memcpy(endpoints[0], pixels[anchor], 4);
    memcpy(endpoints[1], pixels[other], 4);

    for (i = 0; i < 16; i++) {
        int dot = 0;
        unsigned norm = 0;
        for (c = 0; c < 4; c++) {
            int delta = (int)endpoints[1][c] - endpoints[0][c];
            dot += ((int)pixels[i][c] - endpoints[0][c]) * delta;
            norm += (unsigned)(delta * delta);
        }
        if (dot <= 0 || norm == 0)
            indices[i] = 0;
        else if ((unsigned)dot >= norm)
            indices[i] = 15;
        else
            indices[i] = (unsigned)((15u * (unsigned)dot + norm / 2) / norm);
    }

    /* Pixel zero is the mode 6 anchor with a three-bit index. */
    if (indices[0] >= 8) {
        for (c = 0; c < 4; c++) {
            unsigned char v = endpoints[0][c];
            endpoints[0][c] = endpoints[1][c];
            endpoints[1][c] = v;
        }
        for (i = 0; i < 16; i++)
            indices[i] = 15 - indices[i];
    }

    memset(dst, 0, 16);
    put_bits(dst, &pos, 1u << 6, 7);
    for (c = 0; c < 4; c++) {
        put_bits(dst, &pos, endpoints[0][c] >> 1, 7);
        put_bits(dst, &pos, endpoints[1][c] >> 1, 7);
    }
    put_bits(dst, &pos, endpoints[0][0] & 1, 1);
    put_bits(dst, &pos, endpoints[1][0] & 1, 1);
    put_bits(dst, &pos, indices[0], 3);
    for (i = 1; i < 16; i++)
        put_bits(dst, &pos, indices[i], 4);
    return 16;
}
