/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (downscales raw BGRA frames)
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

/*! \brief rescales the image buffer at given frame width/height down by 2.
 *
 *  \p buffer the buffer containing the frame to scale down. the result will be stored in this buffer, as well.
 *  \p width image width of given frame. the result will be (width / 2).
 *  \p height image height of given frame. the result will be (height / 2).
 */
void scaleBGRA(unsigned char *buffer, uint32_t width, uint32_t height) {
	const int pixelStride = 4;
	const int lineStride = width * pixelStride;

	// C(x0, y0, cp) will return the colour component (cp) of pixel at coordinate (x0:y0)
	#define C(x0, y0, cp) (buffer[(y + y0)*lineStride + (x + x0)*pixelStride + cp])

	for (uint32_t y = 0; y < height; y += 2) {
		for (uint32_t x = 0; x < width; x += 2) {
			// collect 2x2 pixel (ignore alpha)
			const uint8_t p[2][2][3] = {
				//top-left								top-right
				{ {C(0, 0, 0), C(0, 0, 1), C(0, 0, 2)}, {C(1, 0, 0), C(1, 0, 1), C(1, 0, 2)} },
				//bottom-left							bottom-right
				{ {C(0, 1, 0), C(0, 1, 1), C(0, 1, 2)}, {C(1, 1, 0), C(1, 1, 1), C(1, 1, 2)} },
			};

			// average the colours components
			const uint8_t r[3] = {
				(p[0][0][0] + p[1][0][0] + p[0][1][0] + p[1][1][0]) / 4, // B
				(p[0][0][1] + p[1][0][1] + p[0][1][1] + p[1][1][1]) / 4, // G
				(p[0][0][2] + p[1][0][2] + p[0][1][2] + p[1][1][2]) / 4, // R
			};

			// write to new destination
			unsigned char *tmp = buffer + width * y + 2 * x;
			*tmp++ = r[0]; // B
			*tmp++ = r[1]; // G
			*tmp++ = r[2]; // R
			// do not copy the alpha value as it shall always be 0xFF
		}
	}
	#undef C
}

// vim:ai:noet:ts=4
