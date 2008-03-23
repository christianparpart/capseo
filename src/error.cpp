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

#include <string.h>
#include <errno.h>

/*! \brief converts an error code into a human readable message.
 *  \param AErrorCode the error code to be translated.
 *  \return a human readable message describing the error.
 */
char *CapseoErrorString(int AErrorCode) {
	switch (AErrorCode) {
		case CAPSEO_E_SUCCESS:
			return "Success";
		case CAPSEO_E_SYSTEM:
			return strerror(errno);
		case CAPSEO_E_GENERAL:
			return "General error";
		case CAPSEO_E_INTERNAL:
			return "Internal error";
		case CAPSEO_E_NOT_SUPPORTED:
			return "Not supported/implemented";
		case CAPSEO_E_INVALID_ARGUMENT:
			return "Invalid argument";
		case CAPSEO_E_INVALID_HEADER:
			return "Invalid header";
		default:
			return "Unknown error code";
	}
}

// vim:ai:noet:ts=4:nowrap
