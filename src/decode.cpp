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

#include <assert.h>

/*! \brief decodes a capseo stream header from the bitstream
 *  \param inbuf bitstream input packet
 *  \param inlen bitstream input packet size
 *  \param out capseo information found in header will be stored here
 *  \retval CAPSEO_SUCCESS successfully decoded.
 *  \retval CAPSEO_E_INVALID_ARGUMENT something went wrong with your arguments you passed,
 *  	mostly because the bitstream is invalid.
 *  \retval CAPSEO_E_NOT_SUPPORTED
 *
 *  \code
 *  	ogg_stream_state ogg;
 *  	ogg_packet packet;
 *
 *  	ogg_stream_packetpeek(&ogg, &packet); // peek packet to test wether this is a capseo stream header
 *
 *  	capseo_info_t info;
 *  	int error = CapseoDecodeStreamHeader(packet.packet, packet.bytes, &info);
 *  \endcode
 */
int CapseoDecodeStreamHeader(uint8_t *inbuf, int inlen, capseo_info_t *out) {
	if (inlen != sizeof(TCapseoStreamHeader))
		return CAPSEO_E_INVALID_ARGUMENT;

	TCapseoStreamHeader *header = (TCapseoStreamHeader *)inbuf;
	if (header->magic[0] != 'C' || header->magic[1] != 'P' || header->magic[2] != 'S')
		return CAPSEO_E_INVALID_ARGUMENT;

	// now we may be sure, that we're talking about a header that belongs to us

	if (header->magic[3] != 0x01) // only codec revision 0 supported
		return CAPSEO_E_NOT_SUPPORTED;

	out->width = ntohl(header->width);
	out->height = ntohl(header->height);
	out->scale = ntohl(header->scale);
	out->fps = ntohl(header->fps);

	out->encoded_video_fmt = ntohl(header->video_format);
	out->encoded_cursor_fmt = ntohl(header->cursor_format);

	switch (out->encoded_cursor_fmt) {
		case CAPSEO_FORMAT_ENCORE_QLZARGB:
		case CAPSEO_FORMAT_ENCORE_ARGB:
			out->cursor_format = CAPSEO_FORMAT_ARGB;
			break;
	}

	return CAPSEO_SUCCESS;
}

/*! \brief decodes a single frame.
 *  \param cs the codec handle
 *  \param inbuf contains the capseo-encoded frame data.
 *  \param inlen buffer length of the encoded frame data, \p inbuf.
 *  \param cursor if zero, no cursor will be decoded into the frame (in case one is in the frame), otherwise it will do so
 *  \param outbuf the buffer to store the decoded frame to
 *  \param outid the time based frame ID will be stored in *outid
 *  \retval CAPSEO_SUCCESS success.
 *  \retval CAPSEO_E_NOT_SUPPORTED reequested output format not implemented
 *
 *  \remarks Currently only one output format is supported. \b CAPSEO_FORMAT_YUV420.
 */
int CapseoDecodeFrame(capseo_t *cs, uint8_t *inbuf, int inlen, int cursor, capseo_frame_t *out) {
	if (cs->info.format != CAPSEO_FORMAT_YUV420)
		return CAPSEO_E_NOT_SUPPORTED;

	uint8_t *inptr = inbuf;

	// decode header
	TCapseoFrameHeader *header = (TCapseoFrameHeader *)inptr;
	out->id = header->id;
	inptr += sizeof(*header);

	// decode video frame
	int size = cs->info.width * cs->info.height * 3 / 2;
	uint8_t *eptr = decode(out->buffer, inptr, size);
	int length = eptr - out->buffer;
	inptr += header->video.length;

	assert(length == cs->info.width * cs->info.height * 3 / 2);

	// decode cursor frame
	if (header->cursor.length) {
		capseo_cursor_t& cursor = cs->priv->FCursor;
		cursor.x = header->cursor.x;
		cursor.y = header->cursor.y;
		cursor.width = header->cursor.width;
		cursor.height = header->cursor.height;
#if 1
		cursor.buffer = cs->priv->encodedBuffer; // use this as tmp storage, as it's currently unused
		eptr = decode(cursor.buffer, inptr, cursor.width * cursor.height * sizeof(uint32_t));
		length = eptr - cursor.buffer;
		assert(length == cursor.width * cursor.height * sizeof(uint32_t));
#else
		cursor.buffer = inptr;
#endif
		drawCursor(cs, out, &cursor, false);

		inptr += header->cursor.length;
	} else {
		// cursor didn't change location/shape
		drawCursor(cs, out, &cs->priv->FCursor, true);
	}

	// finalize with sanity check
	assert(inptr - inbuf == inlen);

	return CAPSEO_SUCCESS;
}

// vim:ai:noet:ts=4:nowrap
