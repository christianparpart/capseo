/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (cpsinfo command line tool)
//
//  Authors:
//      Copyright (c) 2007 by Christian Parpart <trapni@gentoo.org>
//
//  This file as well as its whole library is licensed under
//  the terms of GPL. See the file COPYING.
//
/////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdarg.h>
#include <capseo.h>
#include <string.h>

/* Requirements:
 * - average FPS
 * - MiB/s
 * - duration: HH:MM:SS
 * - and everything the upcoming header will inform us (once implemented):
 *   - comments (key, value pairs)
 *   - FPS hint, ...
 */

int die(const char *msg, ...) {//{{{
	char buf[1024];
	va_list vp;

	va_start(vp, msg);
	vsnprintf(buf, sizeof(buf), msg, vp);
	va_end(vp);

	fprintf(stderr, "error: %s\n", buf);
	return 1;
}//}}}

int main(int argc, char *argv[]) {
	if (argc != 2)
		return die("Invalid argument count");

	capseo_info_t info;
	bzero(&info, sizeof(info));
	info.mode = CAPSEO_MODE_DECODE;
	info.format = CAPSEO_FORMAT_YUV420;

	char *fileName = argv[1];

	capseo_stream_t *stream;
	if (int error = CapseoStreamCreateFileName(CAPSEO_MODE_DECODE, &info, fileName, &stream))
		return die("Could not open stream: code %d\n", error);

	printf("media:             : %s\n", fileName);
	printf("\n");

	printf("video:\n");
	printf("  frame width      : %d\n", info.width);
	printf("  frame height     : %d\n", info.height);
	printf("  frame scale      : %d\n", info.scale);
	printf("  fps hint         : ");
	if (info.fps)
		printf("%d\n", info.fps);
	else
		printf("n/a\n");
	printf("\n");

	printf("cursor\n");
	printf("  present          : %s\n", info.cursor_format != 0 ? "likely" : "unlikely");
	printf("\n");

	CapseoStreamDestroy(stream);

	return 0;
}
