/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (cpsrecode re-encodes capseo encoded files into y4m format for further reuse)
//
//  Authors:
//      Copyright (c) 2007 by Christian Parpart <trapni@gentoo.org>
//
//  This file as well as its whole library is licensed under
//  the terms of GPL. See the file COPYING.
//
/////////////////////////////////////////////////////////////////////////////
#include <capseo.h>

// XXX pay attention: this tool is not yet supposed to work yet.
// XXX e.g. you just can't pass YUV420 buffers to OpenGL

#include <arpa/inet.h> // ntohl()
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <GL/glx.h>
#include <stdarg.h>

// {{{ Xlib funcs
static Bool waitMapNotify(Display *dpy, XEvent *e, char *arg) {
	return dpy && (e->type == MapNotify) && (e->xmap.window == (Window) arg);
}

static Bool waitConfigureNotify(Display *dpy, XEvent *e, char *arg) {
	return dpy && (e->type == ConfigureNotify) && (e->xconfigure.window == (Window) arg);
}

static Window createWindow(Display *dpy, int width, int height) {
	GLXFBConfig *fbc;
	XVisualInfo *vi;
	Colormap cmap;
	XSetWindowAttributes swa;
	GLXContext cx;
	XEvent event;
	int nElements;

	int attr[] = { GLX_DOUBLEBUFFER, True, None };

	fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), attr, &nElements);
	vi = glXGetVisualFromFBConfig(dpy, fbc[0]);

	cx = glXCreateNewContext(dpy, fbc[0], GLX_RGBA_TYPE, 0, GL_FALSE);
	cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);

	swa.colormap = cmap;
	swa.border_pixel = 0;
	swa.event_mask = 0;
	Window win = XCreateWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);
	XSelectInput(dpy, win, StructureNotifyMask | KeyPressMask | KeyReleaseMask);
	XMapWindow(dpy, win);
	XIfEvent(dpy, &event, waitMapNotify, (char *)win);

	XMoveWindow(dpy, win, 100, 100);
	XIfEvent(dpy, &event, waitConfigureNotify, (char *)win);

	glXMakeCurrent(dpy, win, cx);
	return win;
}
// }}}

int die(const char *msg, ...) {//{{{
	char buf[1024];
	va_list vp;

	va_start(vp, msg);
	vsnprintf(buf, sizeof(buf), msg, vp);
	va_end(vp);

	fprintf(stderr, "error: %s\n", buf);
	return 1;
}//}}}

inline uint64_t utime() {
	struct timeval tv;
	gettimeofday(&tv, 0);
	return uint64_t(tv.tv_sec * 1000000 + tv.tv_usec);
}

capseo_stream_t *stream = 0;	//!< capseo stream to be played
capseo_info_t info;				//!< capseo out parametera (header informations)

void DrawFrame(capseo_frame_t *frame) {
	glRasterPos2i(-1, -1);
	glDrawPixels(info.width, info.height, GL_BGRA, GL_UNSIGNED_BYTE, frame->buffer);
}

int main(int argc, char *argv[]) {
#if !defined(I_WANT_TO_CONTRIBUTE)
	die("This piece of cursed code is under development!\n\tPlease use `cpsrecode movie.cps | mplayer -` to view your movie.");
	return 42;
#endif
	bzero(&info, sizeof(capseo_info_t));
	const char *fileName = argc >= 2 ? argv[1] : "/tmp/example.captury";

	capseo_stream_t *stream;
	if (int error = CapseoStreamCreateFileName(CAPSEO_MODE_DECODE, &info, fileName, &stream))
		return die("coult not open input stream: %s (error code: %d)", fileName, error);

//	int fps = 25;
//	float interval = 1000000.0 / (1.0 * fps);

	printf("input file: %s\n", fileName);
	printf("geometry: %dx%d\n", info.width, info.height);

	Display *dpy = XOpenDisplay(0);
	Window win = createWindow(dpy, info.width, info.height);

	unsigned frameCount = 0;
	uint64_t elapsed = utime();

	for (uint64_t now = utime(), last = now, lastID = 0; true; now = utime()) {
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		capseo_frame_t *frame;
		if (int error = CapseoStreamDecodeFrame(stream, &frame, true))
			return die("Frame Decode error: code: %d\n", error);

		uint64_t currID = frame->id;
		uint64_t elapsed = now - last; // time elapsed since last frame draw
		uint64_t interval = currID - lastID; // expected interval between last and current frame to usleep()

		if (elapsed < interval && frameCount)
			usleep(interval - elapsed);

		lastID = frame->id;
		last = now;

		++frameCount;
		DrawFrame(frame);
		glXSwapBuffers(dpy, win);
	}

	printf("\n");

	elapsed = utime() - elapsed;

	printf("frames %d, elapsed: %.2fs, fps %.2f\n",
		frameCount, elapsed / 1000000.0, 1000000.0 * frameCount / elapsed
	);

	return 0;
}
