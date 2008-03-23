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

#define u32(ptr) (*(uint32_t *)(ptr))

/*! \brief encodes given src and stores result into dst.
 *
 *  \p dst the resulting encoded buffer will be stored there
 *  \p src the input buffer to encode
 *  \p size input (src) buffer size
 *  \return pointer to last encoded byte
 *
 *  \remarks based on quicklz (http://www.quicklz.com)
 *           Copyright 2006, Lasse Reinhold (lar@quicklz.com)
 */
uint8_t *encode(uint8_t *dst, uint8_t *src, uint32_t size) {
	const uint8_t *end = src + size;
	const uint8_t **hashtable = (const uint8_t **)(dst + size + 36000 - sizeof(uint8_t *) * 4096);
	uint8_t *cword_ptr = dst++;
	uint8_t cword_val = 0;
	uint8_t cword_counter = 8;

	for (int i = 0; i < 4096; ++i)
		hashtable[i] = src;

	while (src < end - sizeof(uint32_t)) {
		if (u32(src) == u32(src + 1)) { /* RLE sequence */
			uint32_t fetch = u32(src);
			src += sizeof(uint32_t);
			const uint8_t *orig = src;
			while (fetch == u32(src) && src < orig + (0x0fff << 2) - sizeof(uint32_t) && src < end - sizeof(uint32_t))
				src += sizeof(uint32_t);
			uint32_t len = (src - orig) / sizeof(uint32_t);
			*dst++ = (uint8_t) 0xf0 | (len >> 8);
			*dst++ = (uint8_t) len;
			*dst++ = (uint8_t) (fetch & 0xff);
			cword_val = (cword_val << 1) | 1;
		} else {
			/* fetch source data and update hash table */
			uint32_t fetch = ntohl(u32(src));
			uint32_t hash = ((fetch >> 20) ^ (fetch >> 8)) & 0x0fff;
			const uint8_t *o = hashtable[hash];
			hashtable[hash] = src;

			uint32_t offset = (uint32_t) (src - o);
			if (offset <= 131071 && offset > 3 && ((ntohl(u32(o)) ^ ntohl(u32(src))) & 0xffffff00) == 0) {
				if (o[3] != src[3]) {
					if (offset <= 127) { /* LZ match */
						*dst++ = offset;
						cword_val = (cword_val << 1) | 1;
						src += 3;
					} else if (offset <= 8191) { /* LZ match */
						*dst++ = (uint8_t) 0x80 | offset >> 8;
						*dst++ = (uint8_t) offset;
						cword_val = (cword_val << 1) | 1;
						src += 3;
					} else { /* literal */
						*dst++ = *src++;
						cword_val = (cword_val << 1);
					}
				} else { /* LZ match */
					cword_val = (cword_val << 1) | 1;
					uint32_t len = 0;

					while (*(o + len + 4) == *(src + len + 4) && len < (1 << 11) - 1 && src + len + 4 < end - sizeof(uint32_t))
						++len;
					src += len + 4;
					if (len <= 7 && offset <= 1023) { /* 10bits offset, 3bits length */
						*dst++ = (uint8_t) 0xa0 | (len << 2) | (offset >> 8);
						*dst++ = (uint8_t) offset;
					} else if (len <= 31 && offset <= 65535) { /* 16bits offset, 5bits length */
						*dst++ = (uint8_t) 0xc0 | len;
						*dst++ = (uint8_t) (offset >> 8);
						*dst++ = (uint8_t) offset;
					} else { /* 17bits offset, 11bits length */
						*dst++ = (uint8_t) 0xe0 | (len >> 7);
						*dst++ = (uint8_t) (len << 1) | (offset >> 16);
						*dst++ = (uint8_t) (offset >> 8);
						*dst++ = (uint8_t) offset;
					}
				}
			} else { /* literal */
				*dst++ = *src++;
				cword_val = (cword_val << 1);
			}
		}

		--cword_counter;
		if (cword_counter == 0) { /* store control word */
			*cword_ptr = cword_val;
			cword_counter = 8;
			cword_ptr = dst++;
		}
	}

	/* save last source bytes as literals */
	while (src < end) {
		*dst++ = *src++;
		cword_val = (cword_val << 1);
		--cword_counter;
		if (cword_counter == 0) {
			*cword_ptr = cword_val;
			cword_counter = 8;
			cword_ptr = dst++;
		}
	}

	if (cword_counter > 0)
		cword_val = (cword_val << cword_counter) | (1 << (cword_counter - 1));

	*cword_ptr = cword_val;

	return dst;
}

// vim:ai:noet:ts=4:nowrap
