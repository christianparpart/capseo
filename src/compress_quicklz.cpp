/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (Capseo Implementation Independant Compression API)
//
//  Authors:
//      Copyright (c) 2007-2008 by Christian Parpart <trapni@gentoo.org>
//
//  This file as well as its whole library is licensed under
//  the terms of GPL. See the file COPYING.
//
/////////////////////////////////////////////////////////////////////////////
#include "compress.h"

extern "C" {
	#include "quicklz.h"
}

#include <string.h>
#include <stdlib.h>

void *CompressorCreate() {
	char *scratch = (char *)malloc(QLZ_SCRATCH_COMPRESS);
	memset(scratch, 0, QLZ_SCRATCH_COMPRESS);

	return scratch;
}

int Compress(void *AHandle, void *AInput, int AInputSize, void *AOutput) {
	return qlz_compress((const char *)AInput, (char *)AOutput, AInputSize, (char *)AHandle);
}

void CompressorDestroy(void *AHandle) {
	free(AHandle);
}

void *DecompressorCreate() {
	char *scratch = (char *)malloc(QLZ_SCRATCH_DECOMPRESS);
	memset(scratch, 0, QLZ_SCRATCH_DECOMPRESS);

	return scratch;
}

int Decompress(void *AHandle, void *AInput, void *AOutput) {
	return qlz_decompress((const char *)AInput, (char *)AOutput, (char *)AHandle);
}

void DecompressorDestroy(void *AHandle) {
	free(AHandle);
}
