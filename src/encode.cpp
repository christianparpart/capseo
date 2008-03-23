/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (Core Frame Encoder API implementation)
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
#include <sys/time.h>
#include <string.h>
#include <assert.h>
#include <math.h>

/*! \brief Encodes a stream header that represents given codec handle.
 * \param cs the codec handle.
 * \param buffer will point to buffer holding the encoded streamheader
 * \param buflen the actual header length of the encoded header.
 * \retval CAPSEO_SUCCESS Success.
 * \code
 * 	capseo_t *cs; // your capseo handle
 * 	int fd; // your file descriptor where you want to store the stream to be encoded
 *
 * 	uint8_t *buffer;
 * 	int buflen;
 * 	CapseoEncodeStreamHeader(cs, &buffer, &buflen);
 *
 * 	write(fd, buffer, buflen);
 * \endcode
 */
int CapseoEncodeStreamHeader(capseo_t *cs, uint8_t **buffer, int *buflen) {
	TCapseoStreamHeader header;
	bzero(&header, sizeof(header));

	header.magic[0] = 'C';
	header.magic[1] = 'P';
	header.magic[2] = 'S';
	header.magic[3] = 0X01; // revision

	header.width = htonl(long(cs->info.width / pow(2, cs->info.scale)));
	header.height = htonl(long(cs->info.height / pow(2, cs->info.scale)));
	header.scale = htonl(cs->info.scale);
	header.fps = htonl(cs->info.fps);
	header.video_format = htonl(CAPSEO_FORMAT_ENCORE_QLZYUV420);
	header.cursor_format = htonl(CAPSEO_FORMAT_ENCORE_QLZARGB);

	memcpy(cs->priv->encodedBuffer, &header, sizeof(header));

	*buffer = cs->priv->encodedBuffer;
	*buflen = sizeof(header);

	return CAPSEO_SUCCESS;
}

/*! \brief computes a frame ID for new frames
 *  \param cs handle to codec for which the frame ID is to be computed.
 *  \return the computed frame ID
 *  \see CapseoEncodeFrame()
 *
 *  \remarks Each frame has a unique ID. As this codec does not have a fixed frame rate,
 *           the decoder shall use the time based frame ID to compute when the this frame
 *           shall be rendered.
 */
capseo_frame_id_t CapseoCreateFrameID(capseo_t *cs) {
	struct timeval tv;
	gettimeofday(&tv, 0);
	return capseo_frame_id_t(tv.tv_sec * 1000000 + tv.tv_usec - cs->priv->baseID);
}

/*! \brief encodes given frame.
 *  \param cs the codec handle to operate on
 *  \param frame_in contains the raw frame buffer. its result after leaving this call is <b>undefined</b>.
 *  \param id frame ID that belongs to this frame.
 *  \param outbuf pointer to the encoded buffer will be stored here.
 *  \param outlen encoded frame length
 *  \see CapseoCreateFrameID(), CapseoInit()
 *  \return the size in bytes of the encoded frame, or 0 on error
 *  \code 
 *  	uint8_t *imageFrame = getImageFrame(); // e.g. via glReadPixels() or whatever you capture
 *  	capseo_cursor_t *cursor = getCursorFRame(); // capture cursor data. or just set it to NULL.
 *
 *  	uint8_t *outbuf = 0;
 *  	int outlen = 0;
 *
 *  	int error = CapseoEncodeFrame(cs, imageFrame, CapseoCreateFrameID(), cursorFrame, &outbuf, &outlen);
 *		// (...handle error code...)
 *
 * 		int outfd = STDOUT_FILENO; // write the encoded frame to stdout - usually you write to a file/network stream or so ;)
 *  	sys_write(outfd, outbuf, outlen);
 *  \endcode
 */
int CapseoEncodeFrame(capseo_t *cs, uint8_t *frame_in, capseo_frame_id_t id, capseo_cursor_t *cursor, uint8_t **outbuf, int *outlen) {
	int width = cs->info.width;
	int height = cs->info.height;

	uint8_t *yuvBuffer;
	switch (cs->info.format) {
		case CAPSEO_FORMAT_YUV420:
			if (cs->info.scale != 0) // TODO scaling support
				return CAPSEO_E_NOT_IMPLEMENTED;

			yuvBuffer = (uint8_t *)frame_in;
			break;
		case CAPSEO_FORMAT_BGRA: {
			for (int i = cs->info.scale; i > 0; --i, width /= 2, height /= 2)
				scaleBGRA(frame_in, width, height);

			uint8_t *yuv[3];
			yuv[0] = cs->priv->yuvBuffer;
			yuv[1] = yuv[0] + width * height;
			yuv[2] = yuv[1] + width * height / 4;

			convertBGRAtoYUV420(yuv, (uint8_t *)frame_in, width, height);
			yuvBuffer = yuv[0];
			break;
		}
		default:
			return CAPSEO_E_INVALID_ARGUMENT;
	}

	*outbuf = cs->priv->encodedBuffer;
	*outlen = 0;

	uint8_t *outptr = cs->priv->encodedBuffer;

	// prepare frame header encode
	TCapseoFrameHeader frameHeader;
	bzero(&frameHeader, sizeof(frameHeader));
	frameHeader.id = id;
	outptr += sizeof(frameHeader);
	*outlen += sizeof(frameHeader);

	// encode video frame
	uint8_t *end = encode(outptr, yuvBuffer, width * height * 3 / 2);
	frameHeader.video.length = end - outptr;
	outptr += frameHeader.video.length;
	*outlen += frameHeader.video.length;

	// encode cursor frame
	if (cursor && cursor->buffer) {
		// store cursor image compressed, but keep colour space
#if 1
		end = encode(outptr, cursor->buffer, cursor->width * cursor->height * 4);
		frameHeader.cursor.length = end - outptr;
#else
		frameHeader.cursor.length = cursor->width * cursor->height * 4 * 2;
		memcpy(outptr, cursor->buffer, frameHeader.cursor.length);
#endif
		frameHeader.cursor.x = cursor->x;
		frameHeader.cursor.y = cursor->y;
		frameHeader.cursor.width = cursor->width;
		frameHeader.cursor.height = cursor->height;

		outptr += frameHeader.cursor.length;
		*outlen += frameHeader.cursor.length;

#if 0
		printf("\rcursor origin (%d,%d) extend (%d,%d) (raw: %d, bin: %d)",
			cursor->x, cursor->y, cursor->width, cursor->height,
			cursor->width * cursor->height * 4, frameHeader.cursor.length);
		fflush(stdout);
#endif
	}

	// finalize header encode
	memcpy(cs->priv->encodedBuffer, &frameHeader, sizeof(frameHeader));

	// sanity check
	assert((outptr - cs->priv->encodedBuffer) == *outlen);

	return CAPSEO_SUCCESS;
}

// vim:ai:noet:ts=4:nowrap
