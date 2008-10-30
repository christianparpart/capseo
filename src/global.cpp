/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (Core Frame API implementation)
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
#include "compress.h"

#include <sys/time.h>
#include <string.h>

#include <stdio.h>

const int QUICKLZ_TAIL_SIZE = 36000;	//!< quicklz's buffer tail being used for compressing

int validateEncodeInfo(capseo_info_t *info) {//{{{
	switch (info->format) {
		case CAPSEO_FORMAT_BGRA:
			break; // supported
		case CAPSEO_FORMAT_RGBA:
		case CAPSEO_FORMAT_ARGB:
		case CAPSEO_FORMAT_ABGR:
		case CAPSEO_FORMAT_YUV420:
			return CAPSEO_E_NOT_IMPLEMENTED;
		default:
			return CAPSEO_E_INVALID_ARGUMENT;
	}

	switch (info->cursor_format) {
		case CAPSEO_FORMAT_RGBA:
		case CAPSEO_FORMAT_BGRA:
		case CAPSEO_FORMAT_ARGB:
		case CAPSEO_FORMAT_ABGR:
		case CAPSEO_FORMAT_YUV420:
		case 0: // 0 means no cursor
			break; // valid
		default:
			return CAPSEO_E_INVALID_ARGUMENT;
	}

	{	// validate width/height
		int w = info->width & ~((1 << (info->scale + 1)) - 1);
		int h = info->height & ~((1 << (info->scale + 1)) - 1);

		if (w != info->width || h != info->height)
			return CAPSEO_E_INVALID_ARGUMENT;
	}

	return CAPSEO_SUCCESS;
}//}}}

int validateDecodeInfo(capseo_info_t *info) {//{{{
	switch (info->format) {
		case CAPSEO_FORMAT_BGRA:
		case CAPSEO_FORMAT_RGBA:
		case CAPSEO_FORMAT_ARGB:
		case CAPSEO_FORMAT_ABGR:
			return CAPSEO_E_NOT_IMPLEMENTED;
		case CAPSEO_FORMAT_YUV420:
			break; // supported
		default:
			return CAPSEO_E_INVALID_ARGUMENT;
	}

	switch (info->cursor_format) {
		case CAPSEO_FORMAT_RGBA:
		case CAPSEO_FORMAT_BGRA:
		case CAPSEO_FORMAT_ARGB:
		case CAPSEO_FORMAT_ABGR:
		case CAPSEO_FORMAT_YUV420:
		case 0: // 0 means no cursor
			break; // currently any of those valid, as I even have no clue on how to blend the cursor into an yuv420 image :(((
		default:
			return CAPSEO_E_INVALID_ARGUMENT;
	}

	// validate width/height
	if (info->width || info->height)
		return CAPSEO_E_INVALID_ARGUMENT;

	return CAPSEO_SUCCESS;
}//}}}

/*! \brief initializes the codec handle as requested by given info structure
 *  \param cs capseo codec handle to be initialized
 *  \param info user-land parameters passed to the codec
 *  \retval CAPSEO_SUCCESS success
 *  \retval CAPSEO_E_INVALID_ARGUMENT invalid argument passed (also when something within a passed structure is invalid)
 *  \retval CAPSEO_E_NOT_IMPLEMENTED error (most likely because you passed wrong with/height values
 *  \sa CapseoFinalize(), CapseoEncodeInit()
 */
int CapseoInitialize(capseo_t *cs, capseo_info_t *info) {
	bzero(cs, sizeof(*cs));

	cs->priv = new capseo_private_t;
	bzero(cs->priv, sizeof(*cs->priv));

	info->encoded_video_fmt = CAPSEO_FORMAT_ENCORE_QLZYUV420;
	info->encoded_cursor_fmt = CAPSEO_FORMAT_ENCORE_QLZARGB;

	cs->info = *info;

	switch (info->mode) {
		case CAPSEO_MODE_ENCODE:
			validateEncodeInfo(info);
			cs->priv->compressor = CompressorCreate();
			break;
		case CAPSEO_MODE_DECODE:
			validateDecodeInfo(info);
			cs->priv->compressor = DecompressorCreate();
			break; 
		default:
			return CAPSEO_E_INVALID_ARGUMENT;
	}

	cs->priv->yuvBuffer = new uint8_t[info->width * info->height * 3 / 2];

	cs->priv->encodedBufferLength = info->width * info->height * 4 + QUICKLZ_TAIL_SIZE;
	cs->priv->encodedBuffer = new uint8_t[cs->priv->encodedBufferLength];

	// used for encoding only
	{ 	
		struct timeval tv;
		gettimeofday(&tv, 0);

		cs->priv->baseID = capseo_frame_id_t(tv.tv_sec * 1000000 + tv.tv_usec);
	}

	return CAPSEO_SUCCESS;
}

/*! \brief safely destructs codec handle.
 *  \param cs the handle to be destructed.
 *  \sa CapseoInitialize()
 *
 *  frees all memory safely.
 */
void CapseoFinalize(capseo_t *cs) {
	switch (cs->info.mode) {
		case CAPSEO_MODE_ENCODE:
			CompressorDestroy(cs->priv->compressor);
			break;
		case CAPSEO_MODE_DECODE:
			DecompressorDestroy(cs->priv->compressor);
			break;
	}

	bzero(cs->priv->encodedBuffer, cs->priv->encodedBufferLength);
	delete[] cs->priv->encodedBuffer;
	cs->priv->encodedBuffer = 0;

	delete[] cs->priv->yuvBuffer;
	cs->priv->yuvBuffer = 0;

	bzero(cs->priv, sizeof(*cs->priv));
	delete cs->priv;

	bzero(cs, sizeof(*cs));
}

// vim:ai:noet:ts=4:nowrap
