/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (compresses a frame using quicklz algorithm)
//
//  Authors:
//      Copyright (c) 2007 by Christian Parpart <trapni@gentoo.org>
//
//  This code is based on seom:
//      (http://neopsis.com/projects/seom/)
//
//  This file as well as its whole library is licensed under
//  the terms of GPL. See the file COPYING.
//
/////////////////////////////////////////////////////////////////////////////
#include "capseo_private.h"

#include <string.h>

#define u32(ptr) (*(uint32_t *)(ptr))

static void __memcpy(uint8_t *dst, const uint8_t *src, long len) {
	if (&src[len] > dst) {
		for (const uint8_t *end = dst + len; dst < end; ) {
			*(char *)dst++ = *(char *)src++;
		}
	} else {
		memcpy(dst, src, len);
	}
}

/*! \brief decodes given src and stores result into dst.
 *
 *  \p dst the resulting decoded buffer will be stored there
 *  \p src the input buffer to decode
 *  \p size input (src) buffer size
 *  \return pointer to last decoded byte
 *
 *  \remarks based on quicklz (http://www.quicklz.com)
 *           Copyright 2006, Lasse Reinhold (lar@quicklz.com)
 */
uint8_t *decode(uint8_t *dst, const uint8_t *src, uint32_t size) {
	const uint8_t *end = dst + size;
	uint8_t cword_val = *src++;
	uint8_t cword_counter = 8;

	while (dst < end - sizeof(uint32_t)) {
		if (cword_counter == 0) { /* fetch control word */
			cword_val = *src++;
			cword_counter = 8;
		}

		if (cword_val & (1 << 7)) { /* LZ match or RLE sequence */
			cword_val = (cword_val << 1) | 1;
			--cword_counter;
			if ((src[0] & 0x80) == 0) { /* 7bits offset */
				uint32_t offset = src[0];
				__memcpy(dst, dst - offset, 3);
				dst += 3;
				src += 1;
			} else if ((src[0] & 0x60) == 0) { /* 13bits offset */
				uint32_t offset = ((src[0] & 0x1f) << 8) | src[1];
				__memcpy(dst, dst - offset, 3);
				dst += 3;
				src += 2;
			} else if ((src[0] & 0x40) == 0) { /* 10bits offset, 3bits length */
				uint32_t len = ((src[0] >> 2) & 7) + 4;
				uint32_t offset = ((src[0] & 0x03) << 8) | src[1];
				__memcpy(dst, dst - offset, len);
				dst += len;
				src += 2;
			} else if ((src[0] & 0x20) == 0) { /* 16bits offset, 5bits length */
				uint32_t len = (src[0] & 0x1f) + 4;
				uint32_t offset = (src[1] << 8) | src[2];
				__memcpy(dst, dst - offset, len);
				dst += len;
				src += 3;
			} else if ((src[0] & 0x10) == 0) { /* 17bits offset, 11bits length */
				uint32_t len = (((src[0] & 0x0f) << 7) | (src[1] >> 1)) + 4;
				uint32_t offset = ((src[1] & 0x01) << 16) | (src[2] << 8) | (src[3]);
				__memcpy(dst, dst - offset, len);
				dst += len;
				src += 4;
			} else { /* RLE sequence */
				uint32_t len = ((src[0] & 0x0f) << 8) | src[1];
				uint32_t val = src[2] | (src[2] << 8) | (src[2] << 16) | (src[2] << 24);

				u32(dst) = val;
				dst += sizeof(uint32_t);

				const uint8_t *end = dst + len * sizeof(uint32_t);
				while (dst < end) {
					u32(dst) = val;
					dst += sizeof(uint32_t);
				}
				src += 3;
			}
		} else { /* literal */
			uint8_t index  = cword_val >> 4;
			const uint8_t map[8][2] = { { 4, 0x0f }, { 3, 0x07 }, { 2, 0x03 }, { 2, 0x03 }, { 1, 0x01 }, { 1, 0x01 }, { 1, 0x01 }, { 1, 0x01 } };
			const uint8_t *end = dst + map[index][0];
			while (dst < end)
				*dst++ = *src++;
			cword_counter -= map[index][0];
			cword_val = (cword_val << map[index][0]) | map[index][1];
		}
	}

	while (dst < end) {
		if (cword_counter == 0) {
			cword_counter = 8;
			++src;
		}
		*dst++ = *src++;
		--cword_counter;
	}

	return dst;
}

// vim:ai:noet:ts=4:nowrap
