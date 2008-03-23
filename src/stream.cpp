/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (Stream API implementation)
//
//  Authors:
//      Copyright (c) 2007 by Christian Parpart <trapni@gentoo.org>
//
//  This file as well as its whole library is licensed under
//  the terms of GPL. See the file COPYING.
//
/////////////////////////////////////////////////////////////////////////////
#define _LARGEFILE64_SOURCE (1)

#include "capseo.h"
#include "capseo_private.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

template<typename T>
inline T max(const T& a, const T& b) {
	return a > b ? a : b;
}

inline int CreateEncoderStream(capseo_info_t *info, int fd, capseo_stream_t **stream) {
	capseo_t cs;
	if (int error = CapseoInitialize(&cs, info))
		return error;

	*stream = new capseo_stream_t;
	bzero(*stream, sizeof(**stream));
	(*stream)->frameHandle = cs;
	(*stream)->fd = fd;

	{	// encode stream header
		uint8_t *buffer;
		int buflen;

		CapseoEncodeStreamHeader(&(*stream)->frameHandle, &buffer, &buflen);

		int nwritten = write(fd, buffer, buflen);
		if (nwritten != buflen) {
			CapseoFinalize(&cs);
			delete *stream;
			*stream = 0;

			return CAPSEO_E_SYSTEM; // system error
		}
	}

	const int decodedBufferLength = info->width * info->height * 4;
	for (int i = 0; i < 1; ++i) {
		bzero(&(*stream)->frames[i], sizeof(capseo_frame_t));
		(*stream)->frames[i].buffer = new uint8_t[decodedBufferLength];
		bzero((*stream)->frames[i].buffer, decodedBufferLength);
	}

	(*stream)->encodedHeader = new uint8_t[max(sizeof(TCapseoStreamHeader), sizeof(TCapseoFrameHeader))];

	return CAPSEO_SUCCESS;
}

inline int CreateDecoderStream(capseo_info_t *info, int fd, capseo_stream_t **stream) {
	uint8_t encodedHeader[sizeof(TCapseoStreamHeader)];

	if (read(fd, &encodedHeader, sizeof(encodedHeader)) != sizeof(encodedHeader))
		return CAPSEO_E_SYSTEM;

	if (int error = CapseoDecodeStreamHeader(encodedHeader, sizeof(encodedHeader), info))
		return error;

	capseo_t cs;
	if (int error = CapseoInitialize(&cs, info))
		return error;

	*stream = new capseo_stream_t;
	bzero(*stream, sizeof(**stream));

	const int decodedBufferLength = info->width * info->height * 4;

	(*stream)->fd = fd;
	(*stream)->frameHandle = cs;
	(*stream)->encodedBuffer = new uint8_t[decodedBufferLength + 36000];

	for (int i = 0; i < 2; ++i) {
		bzero(&(*stream)->frames[i], sizeof(capseo_frame_t));
		(*stream)->frames[i].buffer = new uint8_t[decodedBufferLength];
		bzero((*stream)->frames[i].buffer, decodedBufferLength);
	}

	(*stream)->encodedHeader = new uint8_t[max(sizeof(TCapseoStreamHeader), sizeof(TCapseoFrameHeader))];

	return CAPSEO_SUCCESS;
}

/*! \brief creates a stream (based on given local filename) for either encoding or decoding.
 *  \param mode either CAPSEO_MODE_ENCODE for creating an encoding stream,
 *              or CAPSEO_MODE_DECODE for creating a decoding stream.
 *  \param info if creating an encoding stream, it must contain the requested 
 *              values inside the structure filled. 
 *              If it is a decoding stream, than just fill `format`. the others
 *              will get filled for you, so you know what configuration your movie has.
 *  \param fd the file descriptor to read the encoded stream from (or write to).
 *  \return pointer to newly created stream handle.
 *  \see CapseoStreamCreateFd(), CapseoStreamDestroy(), 
 *       CapseoStreamEncodeFrame(), CapseoStreamDecodeFrame()
 */
int CapseoStreamCreateFileName(int mode, capseo_info_t *info, const char *filename, capseo_stream_t **stream) {
	int fd;
	switch (mode) {
		case CAPSEO_MODE_ENCODE:
#if defined(O_LARGEFILE)
			fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE, 0666);
#else
			fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
#endif
			break;
		case CAPSEO_MODE_DECODE:
			fd = open(filename, O_RDONLY);
			break;
		default:
			return CAPSEO_E_INVALID_ARGUMENT;
	}
	if (fd == -1)
		return CAPSEO_E_SYSTEM;

	// TODO set FD_CLOEXEC on fd

	if (int error = CapseoStreamCreateFd(mode, info, fd, stream)) {
		close(fd);
		return error;
	}

	(*stream)->autoCloseFd = true;

	return CAPSEO_SUCCESS;
}

/*! \brief creates a stream (based on given file descriptor) for either encoding or decoding.
 *  \param mode either CAPSEO_MODE_ENCODE for creating an encoding stream,
 *              or CAPSEO_MODE_DECODE for creating a decoding stream.
 *  \param info if creating an encoding stream, it must contain the requested 
 *              values inside the structure filled. 
 *              If it is a decoding stream, than just fill `format`. the others
 *              will get filled for you, so you know what configuration your movie has.
 *  \param fd the file descriptor to read the encoded stream from (or write to).
 *  \return pointer to newly created stream handle.
 *  \see CapseoStreamCreateFileName(), CapseoStreamDestroy(), 
 *       CapseoStreamEncodeFrame(), CapseoStreamDecodeFrame()
 */
int CapseoStreamCreateFd(int mode, capseo_info_t *info, int fd, capseo_stream_t **stream) {
	info->mode = mode;

	switch (mode) {
		case CAPSEO_MODE_ENCODE:
			return CreateEncoderStream(info, fd, stream);
		case CAPSEO_MODE_DECODE:
			return CreateDecoderStream(info, fd, stream);
		default:
			return CAPSEO_E_INVALID_ARGUMENT;
	}
}

/*! \brief safely destructs the stream
 *  \param stream the stream handle to safely destruct.
 *  \see CapseoStreamCreateFileName(), CapseoStreamCreateFd()
 *  \code
 *  	capseo_stream_t *stream = createMyStream();
 *
 *		// ... work with the stream
 *
 *		CapseoStreamDestroy(stream);
 *		stream = NULL;
 *  \endcode
 */
void CapseoStreamDestroy(capseo_stream_t *stream) {
	if (stream->autoCloseFd)
		close(stream->fd);

	int frameCount = stream->frameHandle.info.mode == CAPSEO_MODE_DECODE ? 2 : 1;
	for (int i = 0; i < frameCount; ++i)
		delete[] stream->frames[i].buffer;

	delete[] stream->encodedBuffer; // decoder only, currently
	delete[] stream->encodedHeader; // decoder only, currently

	CapseoFinalize(&stream->frameHandle);

	bzero(stream, sizeof(*stream));
	delete stream;
}

/*! \brief computes time based frame ID for the next frame
 *  \see CapseoCreateFrameID()
 */
capseo_frame_id_t CapseoStreamCreateFrameID(capseo_stream_t *stream) {
	return CapseoCreateFrameID(&stream->frameHandle);
}

/*! \brief encodes given frame
 *  \param stream the stream to write the encoded frame to.
 *  \param frame the raw input frame to encode
 *  \param id the frame ID that belongs to this frame
 *  \retval CAPSEO_SUCCESS success
 *  \retval CAPSEO_SYSTEM system stream write error
 *  \return or any other value returned by CapseoEncodeFrame()
 *  \see CapseoStreamCreateFileName(), CapseoStreamDecodeFrame(), CapseoEncodeFrame()
 */
int CapseoStreamEncodeFrame(capseo_stream_t *stream, uint8_t *frame, capseo_frame_id_t id, capseo_cursor_t *cursor) {
	uint8_t *encodedFrame;
	int length;

	if (int error = CapseoEncodeFrame(&stream->frameHandle, frame, id, cursor, &encodedFrame, &length))
		return error;

	// write encoded frame length (glue code)
	uint32_t frameLength = length;
	int nwritten = write(stream->fd, &frameLength, sizeof(frameLength));
	if (nwritten != sizeof(frameLength))
		return CAPSEO_E_SYSTEM;

	// actually write encoded frame
	nwritten = write(stream->fd, encodedFrame, length);
	if (nwritten != length)
		return CAPSEO_E_SYSTEM;

	return CAPSEO_SUCCESS;
}

/*! \brief decodes a frame from stream
 *  \param stream the stream to decode the frames from
 *  \param frame
 *  \param cursor boolean, decides whether to include the cursor if availabe or not
 *  \return pointer to decoded frame or NULL on decoding error
 *  \see CapseoStreamCreateFileName(), CapseoStreamEncodeFrame(), CapseoStreamDestroy(), CapseoDecodeFrame()
 */ 
int CapseoStreamDecodeFrame(capseo_stream_t *stream, capseo_frame_t **frame, int cursor) {
	// read encoded frame length (glue code)
	uint32_t frameLength;
	int nread = read(stream->fd, &frameLength, sizeof(frameLength));
	if (nread != sizeof(frameLength))
		return nread == 0 
			? CAPSEO_STREAM_END
			: CAPSEO_E_SYSTEM;

	// read encoded frame
	nread = read(stream->fd, stream->encodedBuffer, frameLength);
	if (nread != int(frameLength))
		return CAPSEO_E_SYSTEM;

	// choose frame storage
	*frame = &stream->frames[stream->processedFrames++ % 2];

	// actually decode frame
	return CapseoDecodeFrame(&stream->frameHandle, stream->encodedBuffer, frameLength, cursor, *frame);
}

// vim:ai:noet:ts=4:nowrap
