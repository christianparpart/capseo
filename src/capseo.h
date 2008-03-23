/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (public API)
//
//  Authors:
//      Copyright (c) 2007 by Christian Parpart <trapni@gentoo.org>
//
//  This file as well as its whole library is licensed under
//  the terms of GPL. See the file COPYING.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef capseo_h
#define capseo_h

#include <netinet/in.h> /* I hope there is another way to get uint64_t */
#include <ogg/ogg.h>

/* ----------------------------------------------------------------------- */
/* constants                                                               */

/* codec modes */
#define CAPSEO_MODE_ENCODE		0x1101	/*!< handle is used for encoding */
#define CAPSEO_MODE_DECODE		0x1102	/*!< handle is used for decoding */

/* supported raw frame formats (raw frames and cursors) */
#define CAPSEO_FORMAT_RGBA		0X1201
#define CAPSEO_FORMAT_BGRA		0x1202
#define CAPSEO_FORMAT_ARGB		0x1203
#define CAPSEO_FORMAT_ABGR		0x1204
#define CAPSEO_FORMAT_YUV420	0x1210

/* (ideally) supported encoded frame fromats (frame and cursor) */
#define CAPSEO_FORMAT_ENCORE_QLZYUV420	(0x1301)	/*!< quicklz compressed YUV 4:2:0 */
#define CAPSEO_FORMAT_ENCORE_HUFFYUV	(0x1302)	/*!< HUFFYUV */
#define CAPSEO_FORMAT_ENCORE_MJPEG		(0x1303)	/*!< MJPEG */
#define CAPSEO_FORMAT_ENCORE_ARGB		(0x1350)	/*!< ARGB (e.g. for cursor frames) */
#define CAPSEO_FORMAT_ENCORE_QLZARGB	(0x1351)	/*!< quicklz compressed ARGB */

/* error codes */
#define CAPSEO_E_SUCCESS			(0)			/*!< operation performed as expected */
#define CAPSEO_SUCCESS (CAPSEO_E_SUCCESS)		/*!< operation performed as expected */
#define CAPSEO_E_SYSTEM				(-1)		/*1< system error, see errno */
#define CAPSEO_E_GENERAL			(-2)		/*!< general error */
#define CAPSEO_E_INTERNAL			(-3)		/*!< internal software error */
#define CAPSEO_E_NOT_SUPPORTED		(-4)		/*!< something within the invoked method is not implemented/supported */
#define CAPSEO_E_NOT_IMPLEMENTED	(CAPSEO_E_NOT_SUPPORTED)
#define CAPSEO_E_INVALID_ARGUMENT	(-5)		/*!< some parameter passed (or inside the parameter, if e.g. structure is invalid */
#define CAPSEO_E_ERRNO				(-6)		/*!< system error - use errno to identify the reason */
#define CAPSEO_E_INVALID_HEADER		(-7)		/*!< invalid stream/frame header detected */

#define CAPSEO_STREAM_END			(0x101)		/*!< decoding: stream end reached */

/* ----------------------------------------------------------------------- */
/* stream management                                                       */

struct _capseo_stream_t;
typedef struct _capseo_stream_t capseo_stream_t;

/* ----------------------------------------------------------------------- */
/* frame management                                                        */

struct capseo_private_t;

typedef struct {
	/* general */
	int mode;				/*!< specifies what you are going to do with this handle -
								 either encoding or decoding */
	/* general (video) */
	int width;				/*!< frame width */
	int height;				/*!< frame height */
	int format;				/*!< if encoding: the incoming frame format;
							     if decoding: the requested output format */
	int cursor_format;		/*!< format of the cursor, if provided */
	int fps;				/*!< the fps you think you're encoding with; 
								 keep in mind, this is a VFR codec; so just see this as an 
								 approximate ideal value, which is used for programs like 
								 mencoder/transcode to choose a good default fps 
								 value when recoding */

	/* video encoder only */
	int scale;				/*!< how often shall the frame be down scaled before encoded */

	/* internal: filled out by encoder/decoder automatically */
	int encoded_video_fmt;
	int encoded_cursor_fmt;
} capseo_info_t;

typedef struct {
	struct capseo_private_t *priv;		/*!< implementation relevant opaque data */
	capseo_info_t info;
} capseo_t;

typedef uint64_t capseo_frame_id_t;		/*!< time-based frame ID */

typedef struct _capseo_frame_t {
	capseo_frame_id_t id;				/*!< frame ID */
	uint8_t *buffer;					/*!< raw encoded/decoded buffer */
} capseo_frame_t;

typedef struct _capseo_cursor_t {
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	uint8_t *buffer;
} capseo_cursor_t;

#if defined(__cplusplus)
extern "C" {
#endif

/* ------------------------------------------------------------------------ */
/* stream processing                                                        */

int CapseoStreamCreateFileName(int mode, capseo_info_t *, const char *filename, capseo_stream_t **stream);
int CapseoStreamCreateFd(int mode, capseo_info_t *, int fd, capseo_stream_t **stream);
void CapseoStreamDestroy(capseo_stream_t *);

capseo_frame_id_t CapseoStreamCreateFrameID(capseo_stream_t *);
int CapseoStreamEncodeFrame(capseo_stream_t *cs, uint8_t *frame, capseo_frame_id_t id, capseo_cursor_t *cursor);
int CapseoStreamDecodeFrame(capseo_stream_t *cs, capseo_frame_t **, int cursor);

/* ------------------------------------------------------------------------ */

int CapseoEncodeStreamHeader(capseo_t *cs, uint8_t **buffer, int *buflen);
int CapseoDecodeStreamHeader(uint8_t *inbuf, int inlen, capseo_info_t *out);

//int CapseoEncodeComments(capseo_t *cs, capseo_comments_t *); TODO

/* ------------------------------------------------------------------------ */
/* frame encoding/decoding                                                  */
int CapseoInitialize(capseo_t *cs, capseo_info_t *info);

capseo_frame_id_t CapseoCreateFrameID(capseo_t *cs);
int CapseoEncodeFrame(capseo_t *cs, uint8_t *frame_in, capseo_frame_id_t id, capseo_cursor_t *cursor, uint8_t **outbuf, int *outlen);

int CapseoDecodeFrame(capseo_t *cs, uint8_t *inbuf, int inlen, int cursor, capseo_frame_t *out);

void CapseoFinalize(capseo_t *cs);

/* ------------------------------------------------------------------------ */
/* error handling                                                           */

char *CapseoErrorString(int AErrorCode);

#if defined(__cplusplus)
}
#endif

#endif
