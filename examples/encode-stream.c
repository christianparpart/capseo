/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (capseo stream API example)
//
//  Authors:
//      Copyright (c) 2007 by Christian Parpart <trapni@gentoo.org>
//
//  This file as well as its whole library is licensed under
//  the terms of GPL. See the file COPYING.
//
/////////////////////////////////////////////////////////////////////////////
#include <capseo.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#include <GL/glx.h>

// {{{ X11/GLX
static Bool waitMapNotify(Display *dpy, XEvent *e, char *arg) {
	return dpy && (e->type == MapNotify) && (e->xmap.window == (Window) arg);
}

static Bool waitConfigureNotify(Display *dpy, XEvent *e, char *arg) {
	return dpy && (e->type == ConfigureNotify) && (e->xconfigure.window == (Window) arg);
}

GLXContext cx;
GLXFBConfig *fbc;

static Window createWindow(Display *dpy, int width, int height) {
	XVisualInfo *vi;
	Colormap cmap;
	XSetWindowAttributes swa;
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

	XFree(vi);

	return win;
}
// }}}

int die(const char *fmt, ...) {//{{{
	va_list va;

	fprintf(stderr, "ERROR: ");
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\nI die...\n");
	exit(1);
	return 1; // never reached.
}//}}}

int main(int argc, char *argv[]) {
	Display *dpy = XOpenDisplay(NULL);
	Window win = createWindow(dpy, 400, 400);

	capseo_info_t info;
	bzero(&info, sizeof(info));

	info.width = 400;
	info.height = 400;
	info.format = CAPSEO_FORMAT_BGRA;
	info.scale = argc == 2 ? atoi(argv[1]) : 0;

	capseo_stream_t *stream;
	int error;
	if (error = CapseoStreamCreateFileName(CAPSEO_MODE_ENCODE, &info, "example.cps", &stream))
		return die("Error creating output stream (code %d): %s", error, CapseoErrorString(error));

	const int max = 1000;
	const float step = (float) 1.0 / max;

	uint8_t *frame = (uint8_t *)malloc(info.width * info.height * 4);

	float color[3] = { 0.0, 1.0, 0.5 };
	int i;
	for (i = 0; i < max; ++i) {
		color[0] += step;
		color[1] -= step;

		glClearColor(color[0], color[1], color[2], 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		glReadPixels(0, 0, info.width, info.height, GL_BGRA, GL_UNSIGNED_BYTE, frame);
		CapseoStreamEncodeFrame(stream, frame, CapseoStreamCreateFrameID(stream), NULL);

		glXSwapBuffers(dpy, win);
	}

	free(frame);
	CapseoStreamDestroy(stream);

	XFree(fbc);
	glXDestroyContext(dpy, cx);
	XDestroyWindow(dpy, win);

	XCloseDisplay(dpy);

	return 0;
}
