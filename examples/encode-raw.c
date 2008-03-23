/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (capseo raw frame API example)
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

	info.mode = CAPSEO_MODE_ENCODE;
	info.width = 400;
	info.height = 400;
	info.format = CAPSEO_FORMAT_BGRA;
	info.scale = argc == 2 ? atoi(argv[1]) : 0;

	capseo_t cs;
	CapseoInitialize(&cs, &info);

	int fd = open("./example.cps", O_WRONLY | O_CREAT | O_TRUNC, 0664);
	if (fd == -1)
		return die(strerror(errno));

	struct { uint32_t width, height; } header = { 
		htonl((long)(info.width / pow(2, info.scale))), 
		htonl((long)(info.height / pow(2, info.scale)))
	};
	write(fd, &header, sizeof(header));

	const int max = 1000;
	const float step = (float) 1.0 / max;

	uint8_t *frame_in = (uint8_t *)malloc(info.width * info.height * 4);
	uint8_t *frame_out;
	int i;

	float color[3] = { 0.0, 1.0, 0.5 };
	for (i = 0; i < max; ++i) {
		color[0] += step;
		color[1] -= step;

		glClearColor(color[0], color[1], color[2], 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		{
			glReadPixels(0, 0, info.width, info.height, GL_BGRA, GL_UNSIGNED_BYTE, frame_in);

			capseo_frame_id_t frameID = CapseoCreateFrameID(&cs);
			int length, error;
			if (error = CapseoEncodeFrame(&cs, frame_in, frameID, NULL, &frame_out, &length))
				die("encoding error: code %d: %s\n", error, CapseoErrorString(error));

			write(fd, &frameID, sizeof(frameID));
			write(fd, &length, sizeof(length));
			write(fd, frame_out, length);
		}

		glXSwapBuffers(dpy, win);
	}

	free(frame_in);

	CapseoFinalize(&cs);

	XCloseDisplay(dpy);

	return 0;
}
