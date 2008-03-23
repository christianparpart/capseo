/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (converts BGRA frames to YUV420)
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

#define byte(ptr) ( *(uint8_t *) (ptr) )

#define ri 2
#define gi 1
#define bi 0

#define scale 8
#define f(x) ((uint16_t) ((x) * (1L<<scale) + 0.5))

static const uint16_t m[3][3] = {
	{ f(0.098), f(0.504), f(0.257) },
	{ f(0.439), f(0.291), f(0.148) },
	{ f(0.071), f(0.368), f(0.439) }
};

#define C(xo,yo,s) ( byte(src + (y+yo) * (w*4) + (x+xo) * 4 + s) ) // pixel at (x,y)  component [c] -- bgra

// computes y component
#define SY(xo, yo) \
	byte(yuv[0] + (y + yo) * w + (x + xo)) = \
		(uint8_t) ( \
			( m[0][ri] * p[xo][yo][ri] \
			+ m[0][gi] * p[xo][yo][gi] \
			+ m[0][bi] * p[xo][yo][bi] \
			) >> scale \
		) + 16

void convertBGRAtoYUV420(uint8_t *yuv[3], uint8_t *src, uint32_t w, uint32_t h) {
	for (uint32_t y = 0; y < h; y += 2) {
		for (uint32_t x = 0; x < w; x += 2) {
			// process 4 pixels at a time

			// get 2x2 BGR matrix
			uint8_t p[2][2][3] = {
				// upper left (BGR)					upper right (BGR)
				// lower left (BGR)					lower right (BGR)
				{ { C(0,0,0), C(0,0,1), C(0,0,2) }, { C(0,1,0), C(0,1,1), C(0,1,2) } },
				{ { C(1,0,0), C(1,0,1), C(1,0,2) }, { C(1,1,0), C(1,1,1), C(1,1,2) } },
			};

			// sum up all blue, green and red components
			uint16_t r[3] = {
				p[0][0][0] + p[1][0][0] + p[0][1][0] + p[1][1][0], // blue
				p[0][0][1] + p[1][0][1] + p[0][1][1] + p[1][1][1], // green
				p[0][0][2] + p[1][0][2] + p[0][1][2] + p[1][1][2], // red
			};

			// compute y component for each of the 2x2 pixels
			SY(0, 0);
			SY(1, 0);
			SY(0, 1);
			SY(1, 1);

			// u component
			byte(yuv[1] + y/2*w/2 + x/2) = (uint8_t) ((-m[1][ri] * r[ri] - m[1][gi] * r[gi] + m[1][bi] * r[bi]) >> (scale + 2)) + 128;

			// v component
			byte(yuv[2] + y/2*w/2 + x/2) = (uint8_t) ((m[2][ri] * r[ri] - m[2][gi] * r[gi] - m[2][bi] * r[bi]) >> (scale + 2)) + 128;
		}
	}
}
#undef C
#undef SY

// vim:ai:noet:ts=4:nowrap
