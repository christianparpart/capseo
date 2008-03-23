/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (private API)
//
//  Authors:
//      Copyright (c) 2007 by Christian Parpart <trapni@gentoo.org>
//
//  This file as well as its whole library is licensed under
//  the terms of GPL. See the file COPYING.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef capseo_private_h
#define capseo_private_h

#include "capseo.h"

#define CAPSEO_REVISION (1)

#define CAPSEO_PACKED __attribute__((packed))

struct _capseo_stream_t {
	capseo_t frameHandle;
	capseo_frame_t frames[2];			/*!< currently only used for decoding */

	uint8_t *encodedHeader;				/*!< encoded header (frame/stream) */
	uint8_t *encodedBuffer;				/*!< encoded frame buffer */

	uint64_t processedFrames;			/*!< number of already encoded/decoded frames */

	int fd;								/*!< the actual file descriptor to read from/write to */
	int autoCloseFd;					/*!< if true, the file descriptor will be cllosed 
											 automatically on stream close */
};

struct capseo_private_t {
	uint8_t *yuvBuffer;					/*!< yuv buffer, in case we have to convert */

	uint8_t *encodedBuffer;				/*!< encoded result buffer (frame) */
	unsigned encodedBufferLength;		/*!< length of the result encoded buffer */

	uint64_t baseID;					/*!< used for computing frame ID for the encoded frame */

	// cursor related
	capseo_cursor_t FCursor;
};

struct CAPSEO_PACKED TCapseoStreamHeader {
	uint8_t magic[4];			//!< {'C', 'P', 'S', REVISION}

	uint32_t width;				//!< frame width
	uint32_t height;			//!< frame height
	uint32_t scale;				//!< frame scale value at recording time

	uint32_t fps;				//!< ideal recording fps, this is a hint value

	uint32_t video_format;		//!< video frame format
	uint32_t cursor_format;		//!< cursor format, or 0 if no cursor
};

struct CAPSEO_PACKED TCapseoFrameHeader {
	capseo_frame_id_t id;	//!< frame ID

	struct {
		int32_t length;		//!< encoded video frame length
	} video;

	struct {
		int32_t length;		//!< encoded length in bytes
		int16_t x;			//!< screen x
		int16_t y;			//!< screen y
		int16_t width;		//!< frame width
		int16_t height;		//!< frame height
	} cursor;
};

typedef struct {
	uint8_t y;
	uint8_t u;
	uint8_t v;
} yuv_pixel_t;

typedef struct {
	uint8_t blue;
	uint8_t green;
	uint8_t red;
	uint8_t alpha;
} rgba_pixel_t;

#if defined(__cplusplus)
extern "C" {
#endif

void convertBGRAtoYUV420(uint8_t *yuv[3], uint8_t *source, uint32_t width, uint32_t height);
void scaleBGRA(unsigned char *buffer, uint32_t width, uint32_t height);
uint8_t *encode(uint8_t *dst, uint8_t *src, uint32_t size);
uint8_t *decode(uint8_t *dst, const uint8_t *src, uint32_t size);
void drawCursor(capseo_t *cs, capseo_frame_t *out, capseo_cursor_t *cursor, int AResuseHint);

#if defined(__cplusplus)
}
#endif

#endif
