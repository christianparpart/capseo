/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (Core Frame Decoder API implementation)
//
//  Authors:
//      Copyright (c) 2007 by Christian Parpart <trapni@gentoo.org>
//
//  This file as well as its whole library is licensed under
//  the terms of GPL. See the file COPYING.
//
/////////////////////////////////////////////////////////////////////////////
#include "capseo.h"
#include "capseo_private.h"

#include <stdio.h>
#include <math.h>
#include <assert.h>

// {{{ yuv-helper
#define byte(ptr) ( *(uint8_t *) (ptr) )

#define ri 2
#define gi 1
#define bi 0

#define SCALE 8
#define F(x) ((uint16_t) ((x) * (1L << SCALE) + 0.5))
static const uint16_t m[3][3] = {
	{ F(0.098), F(0.504), F(0.257) },
	{ F(0.439), F(0.291), F(0.148) },
	{ F(0.071), F(0.368), F(0.439) }
};

#define Y_VALUE(r, g, b) (((+m[0][ri] * (r) + m[0][gi] * (g) + m[0][bi] * (b)) >> SCALE) + 16)
#define U_VALUE(r, g, b) (((-m[1][ri] * (r) - m[1][gi] * (g) + m[1][bi] * (b)) >> (SCALE + 2)) + 128)
#define V_VALUE(r, g, b) (((+m[2][ri] * (r) - m[2][gi] * (g) - m[2][bi] * (b)) >> (SCALE + 2)) + 128)

// for debugging only...
static inline void drawLineV(capseo_t *cs, capseo_frame_t *out, int x, int y, int height) {
	for (int y0 = 0; y0 < height; ++y0) {
		out->buffer[(y - y0) * cs->info.width + x] = 0x00;
	}
}

static inline void drawLineH(capseo_t *cs, capseo_frame_t *out, int y, int x, int width) {
	for (int x0 = x; x0 < x + width; ++x0) {
		out->buffer[y * cs->info.width + x0] = 0x00;
	}
}
// }}}

/*! \brief rescales the image buffer at given frame width/height down by 2.
 *
 *  \p buffer the buffer containing the frame to scale down. the result will be stored in this buffer, as well.
 *  \p width image width of given frame. the result will be (width / 2).
 *  \p height image height of given frame. the result will be (height / 2).
 */
static inline void scaleARGB(unsigned char *buffer, uint32_t width, uint32_t height) {
	const int pixelStride = 4;
	const int lineStride = width * pixelStride;

	// C(x0, y0, cp) will return the colour component (cp) of pixel at coordinate (x0:y0)
	#define C(x0, y0, cp) (buffer[(y + y0)*lineStride + (x + x0)*pixelStride + cp])

	for (uint32_t y = 0; y < height; y += 2) {
		for (uint32_t x = 0; x < width; x += 2) {
			// collect 2x2 pixel (ignore alpha)
			const uint8_t p[2][2][4] = {
				//top-left											top-right
				{ {C(0, 0, 0), C(0, 0, 1), C(0, 0, 2), C(0, 0, 3)}, {C(1, 0, 0), C(1, 0, 1), C(1, 0, 2), C(1, 0, 3)} },
				//bottom-left										bottom-right
				{ {C(0, 1, 0), C(0, 1, 1), C(0, 1, 2), C(0, 1, 3)}, {C(1, 1, 0), C(1, 1, 1), C(1, 1, 2), C(1, 1, 3)} },
			};

			// average the colours components
			const uint8_t r[4] = {
				(p[0][0][0] + p[1][0][0] + p[0][1][0] + p[1][1][0]) / 4, // A
				(p[0][0][1] + p[1][0][1] + p[0][1][1] + p[1][1][1]) / 4, // R
				(p[0][0][2] + p[1][0][2] + p[0][1][2] + p[1][1][2]) / 4, // G
				(p[0][0][3] + p[1][0][3] + p[0][1][3] + p[1][1][3]) / 4  // B
			};

			// write to new destination
			unsigned char *tmp = buffer + width * y + 2 * x;
			*tmp++ = r[0]; // A
			*tmp++ = r[1]; // R
			*tmp++ = r[2]; // G
			*tmp++ = r[3]; // B
		}
	}
	#undef C
}

/*! \brief draws the cursor into the frame
 *  \param cs decoder handle
 *  \param out frame to draw the cursor onto
 *  \param cursor the cursor to be drawn 
 *  \param AReuseHint this is a cursor we've already drawn last frame
 *  \remarks currently <b>only</b> ARGB cursors as input are supported - as provided by XFixes X11 extension.
 *  \todo need downscaling of the cursor shape and recomputation of (x,y) 
 *  	  when video has been scaled at encoding time.
 */
void drawCursor(capseo_t *cs, capseo_frame_t *out, capseo_cursor_t *cursor, int AReuseHint) {
	uint32_t *src = (uint32_t *)cursor->buffer;

	const int cx = cursor->x / int(pow(2, cs->info.scale));
	const int cy = cursor->y / int(pow(2, cs->info.scale));

	int cw = cursor->width;
	int ch = cursor->height;

	if (AReuseHint) {
		cw /= int(pow(2, cs->info.scale));
		ch /= int(pow(2, cs->info.scale));
	} else
		for (int i = cs->info.scale; i > 0; --i, cw /= 2, ch /= 2)
			scaleARGB(cursor->buffer, cw, ch);

	uint8_t *yuv[3];
	yuv[0] = out->buffer;
	yuv[1] = yuv[0] + cs->info.width * cs->info.height;
	yuv[2] = yuv[1] + cs->info.width * cs->info.height / 4;

	const int w2 = cs->info.width / 2;

	// {{{ debug: draw box
#if 0
	drawLineH(cs, out, cursor->y, cursor->x, cursor->width);
	drawLineH(cs, out, cursor->y - cursor->height, cursor->x, cursor->width);
	drawLineV(cs, out, cursor->x, cursor->y, cursor->height);
	drawLineV(cs, out, cursor->x + cursor->width, cursor->y, cursor->height);
#endif
	// }}}

	for (int y = 0; y < ch; ++y) {
		for (int x = 0; x < cw; ++x) {
			uint8_t a = (*src >> 24) & 0xFF;

			if (a) {
				a = 255 - a; // invert so that it is true for: (alpha == 255) := invisible

				const int dx = cx + x;
				const int dy = cy - y;

				uint8_t r = ((*src >> 16) & 0xFF);
				uint8_t g = ((*src >> 8) & 0xFF);
				uint8_t b = ((*src) & 0xFF);

				uint8_t *yp = &yuv[0][dy * cs->info.width + dx];
				*yp = *yp * a / 255 + Y_VALUE(r, g, b) * (1 - a/255);

// look ugly :( FIXME
#if 0
				const int dsp = 0; // displacement

				uint8_t *up = &yuv[1][w2*dy/2 + dx/2 + dsp];
				*up = ((*up * 3) + U_VALUE(r, g, b)) / 4;

				uint8_t *vp = &yuv[2][w2*dy/2 + dx/2 + dsp];
				*vp = ((*vp * 3) + V_VALUE(r, g, b)) / 4;
#endif
			}
			++src;
		}
	}
}

// vim:ai:noet:ts=4:nowrap
