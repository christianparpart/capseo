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
#ifndef capseo_compress_h
#define capseo_compress_h

void *CompressorCreate();
int Compress(void *AHandle, void *AInput, int AInputSize, void *AOutput);
void CompressorDestroy(void *AHandle);

// --------------------------------------------------------------------------

void *DecompressorCreate();
int Decompress(void *AHandle, void *AInput, void *AOutput);
void DecompressorDestroy(void *AHandle);

#endif
