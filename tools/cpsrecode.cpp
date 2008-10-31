/////////////////////////////////////////////////////////////////////////////
//
//  CAPSEO - Capseo Video Codec Library
//  $Id$
//  (cpsrecode re-encodes capseo encoded files into y4m format for further reuse)
//
//  Authors:
//      Copyright (c) 2007 by Christian Parpart <trapni@gentoo.org>
//
//  This code is based on seom's seom-filter utility
//      (http://neopsis.com/projects/seom/)
//
//  This file as well as its whole library is licensed under
//  the terms of GPL. See the file COPYING.
//
/////////////////////////////////////////////////////////////////////////////
#define _LARGEFILE64_SOURCE (1)

#include <capseo.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#if THEORA
# include <ogg/ogg.h>
# include <theora/theora.h>
#endif

#if !defined(O_LARGEFILE)
# define O_LARGEFILE (0) // place a stub.
#endif

/// \todo refactor code to be more clean (no ugly global vars)
/// \todo add process info (completion ETA, %, ...)

class IEncoder {//{{{
public:
	virtual ~IEncoder() {}

	virtual void initialize() = 0;
	virtual unsigned writeFrame(capseo_frame_t * frame, bool last = false) = 0;
	virtual void finalize() = 0;
};//}}}

int fps = 25;					//!< average fps to re-encode with
int inputFd = -1;				//!< file descriptor, where to read the capseo video from
int outputFd = -1;				//!< file descriptor, where to write the re-encoded video to
capseo_stream_t *stream = 0;	//!< capseo input stream handle
capseo_info_t info;				//!< capseo out parameters
IEncoder *encoder = 0;			//!< the encoder to use
bool FDupFrame = false;			//!< true when frame is being reused
int verbose = 1;				//!< verbosity level (0 = quiet)

int die(const char *fmt, ...) {//{{{
	va_list va;

	fprintf(stderr, "ERROR: ");
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n");
	exit(1);
	return 1; // never reached.
}//}}}

void printHelp() {//{{{
	printf(
		"capseo recode, version %s\n"
		"\t-r:  frames per second\n"
		"\t-i:  input filename (or - for stdin)\n"
#if THEORA
		"\t-c:  specify output codec to use (y4m, theora)\n"
#else
		"\t-c:  specify output codec to use (y4m only)\n"
#endif
		"\t-o:  output filename (or - for stdout)\n"
		"\t-q:  be quiet when processing\n"
		"\t-h:  print help text\n",
		VERSION
	);
}//}}}

template<typename T>
inline T diff(const T& t1, const T& t2) {
	return t1 < t2 ? t2 - t1 : t1 - t2;
}

class TY4MEncoder : public IEncoder {//{{{
public:
	virtual void initialize() {
		char header[128];
		int n = snprintf(header, sizeof(header), "YUV4MPEG2 W%d H%d F%d:1 Ip\n", info.width, info.height, fps);
		write(outputFd, header, n);
	}

	virtual unsigned writeFrame(capseo_frame_t *frame, bool /*last*/) {
		unsigned nwritten = 0;
		static const char header[] = "FRAME\n";
		nwritten += write(outputFd, header, sizeof(header) - 1);

		uint8_t *buffer = frame->buffer;

		// write each freame row up-side-down
		for (int y = info.height - 1; y >= 0; --y)
			nwritten += write(outputFd, buffer + y * info.width, info.width);

		buffer += info.width * info.height;

		for (int i = 0; i < 2; ++i) {
			for (int y = (info.height / 2) - 1; y >= 0; --y)
				nwritten += write(outputFd, buffer + y * (info.width / 2), (info.width / 2));

			buffer += info.width * info.height / 4;
		}

		return nwritten;
	}

	virtual void finalize() {
	}
};//}}}

#if THEORA
class TOggTheoraEncoder : public IEncoder {//{{{
private:
	ogg_stream_state FVideoStream;
	ogg_packet FVideoPacket;
	theora_state FVideoCodec;
	yuv_buffer yuv;
	unsigned char *buffer;
	unsigned char *tmp;			//!< for v-flipping

//	ogg_stream_state FAudioStream;
//	ogg_packet FAudioPacket;
//	vorbis_state FAudioCodec;

public:
	virtual void initialize() {
		tmp = new unsigned char[info.width];
		buffer = new unsigned char[info.width * info.height * 4];

		yuv.y_width = ((info.width + 15) >> 4) << 4;
		yuv.y_height = ((info.height + 15) >> 4) << 4;
		yuv.y_stride = yuv.y_width;

		yuv.uv_width = yuv.y_width / 2;
		yuv.uv_stride = yuv.uv_width;
		yuv.uv_height = yuv.y_height / 2;

		ogg_stream_init(&FVideoStream, 1); // video stream id gets fixed value 1, so possible audio stream might get fixed id 2 e.g.

		theoraInit();
		theoraComments();
		theoraTables();
	}

	#define YUV_Y(line) (y + ((line) * info.width))
	#define YUV_U(line) (u + ((line) * info.width / 2))
	#define YUV_V(line) (v + ((line) * info.width / 2))

	unsigned char *flipV(capseo_frame_t *frame) {
		if (FDupFrame)
			return frame->buffer;

		unsigned char *y = frame->buffer;
		unsigned char *u = y + info.width * info.height;
		unsigned char *v = u + info.width * info.height / 4;

		const int w2 = info.width / 2;

		for (int y0 = 0, y1 = info.height - 1; y0 < info.height / 2; ++y0, --y1) {
			// vflip y
			memcpy(tmp, YUV_Y(y0), info.width);
			memcpy(YUV_Y(y0), YUV_Y(y1), info.width);
			memcpy(YUV_Y(y1), tmp, info.width);
		}

		for (int y0 = 0, y1 = (info.height/2 - 1); y0 < info.height / 4; ++y0, --y1) {
			// vflip u
			memcpy(tmp, YUV_U(y0), w2);
			memcpy(YUV_U(y0), YUV_U(y1), w2);
			memcpy(YUV_U(y1), tmp, w2);

			// vflip v
			memcpy(tmp, YUV_V(y0), w2);
			memcpy(YUV_V(y0), YUV_V(y1), w2);
			memcpy(YUV_V(y1), tmp, w2);
		}

		return frame->buffer;
	}

	virtual unsigned writeFrame(capseo_frame_t *frame, bool last) {
		yuv.y = flipV(frame);
		yuv.u = yuv.y + info.width * info.height;
		yuv.v = yuv.u + info.width * info.height / 4;

		int rv = theora_encode_YUVin(&FVideoCodec, &yuv);
		checkError(rv, "theora_encode_YUVin");

		rv = theora_encode_packetout(&FVideoCodec, false, &FVideoPacket);
		checkError(rv, "theora_encode_packetout");

		if (last)
			FVideoPacket.e_o_s = 1;

		ogg_stream_packetin(&FVideoStream, &FVideoPacket);
		return flushOnce();
	}

	virtual void finalize() {
		flushAll();

		theora_clear(&FVideoCodec);
		ogg_stream_clear(&FVideoStream);
//		ogg_stream_destroy(&FVideoStream);

		delete[] buffer;
		delete[] tmp;
	}

private:
	void theoraInit() {
		theora_info ti;
		theora_info_init(&ti);

		ti.width = info.width;
		ti.height = info.height;
		ti.frame_width = info.width;
		ti.frame_height = info.height;
		ti.offset_x = 0;
		ti.offset_y = 0;

		ti.fps_numerator = fps;
		ti.fps_denominator = 1;

		ti.aspect_numerator = 1;
		ti.aspect_denominator = 1;

		ti.dropframes_p = 1;
		ti.quick_p = 0;
		ti.keyframe_auto_p = 1;
		ti.keyframe_frequency = 64;
		ti.keyframe_frequency_force = 64;
		ti.keyframe_data_target_bitrate = (ogg_uint32_t)(ti.target_bitrate * 1.5);
		ti.keyframe_auto_threshold = 80;
		ti.keyframe_mindistance = 8;
		ti.quality = 63; // 0..63
		ti.noise_sensitivity = 1;

		theora_encode_init(&FVideoCodec, &ti);
		theora_info_clear(&ti);

		int rv = theora_encode_header(&FVideoCodec, &FVideoPacket);
		checkError(rv, "theora_encode_header");

		ogg_stream_packetin(&FVideoStream, &FVideoPacket);

		flushOnce();
	}

	void theoraComments() {
		theora_comment tc;
		theora_comment_init(&tc);

		static struct {
			const char *key;
			const char *value;
		} comments[] = {
			{ "ENCODER", "cpsrecode" },
			{ "SOURCE", "captury" },
			{ 0, 0 }
		};

		for (int i = 0; comments[i].key; ++i)
			theora_comment_add_tag(&tc, (char *)comments[i].key, (char *)comments[i].value);

		int rv = theora_encode_comment(&tc, &FVideoPacket);
		checkError(rv, "theora_encode_comment");

		ogg_stream_packetin(&FVideoStream, &FVideoPacket);
	}

	void theoraTables() {
		int rv = theora_encode_tables(&FVideoCodec, &FVideoPacket);
		checkError(rv, "theora_encode_tables");

		ogg_stream_packetin(&FVideoStream, &FVideoPacket);
		flushAll();
	}

	const char *theoraErrorStr(int rc) {
		switch (rc) {
			case OC_FAULT:
				return "General Error";
			case OC_EINVAL:
				return "Library encountered invalid internal data";
			case OC_DISABLED:
				return "Requested action is disabled";
			case OC_BADHEADER:
				return "Header packet was corrupt/invalid";
			case OC_NOTFORMAT:
				return "Packet is not theora packet";
			case OC_VERSION:
				return "Bitstream version is not handled";
			case OC_IMPL:
				return "Feature or action not implemented";
			case OC_BADPACKET:
				return "Packet is corrupt";
			case OC_NEWPACKET:
				return "Packet is an (ignorable) unhandled extension";
			case OC_DUPFRAME:
				return "Packet is a dropped frame";
			default:
				return "Unknown theora error";
		}
	}

	void checkError(int rv, const char *fname) {
		if (rv < 0) {
			fprintf(stderr, "theora error(%s): %d: %s\n", fname ? fname : "unknown", rv, theoraErrorStr(rv));
			exit(1);
		}
	}

	unsigned flushOnce() {
		unsigned nwritten = 0;
		ogg_page page;

		if (ogg_stream_pageout(&FVideoStream, &page)) {
			nwritten += write(outputFd, page.header, page.header_len);
			nwritten += write(outputFd, page.body, page.body_len);
		}
		return nwritten;
	}

	unsigned flushAll() {
		unsigned nwritten = 0;
		ogg_page page;

		while (ogg_stream_pageout(&FVideoStream, &page)) {
			nwritten += write(outputFd, page.header, page.header_len);
			nwritten += write(outputFd, page.body, page.body_len);
		}

		if (ogg_stream_flush(&FVideoStream, &page)) {
			nwritten += write(outputFd, page.header, page.header_len);
			nwritten += write(outputFd, page.body, page.body_len);
		}

		return nwritten;
	}
};//}}}
#endif

//{{{ TPerformanceCounter<TValue, PERIOD>
template<typename TValue = double, const unsigned PERIOD = 60>
class TPerformanceCounter {
private:
	unsigned FCounter[PERIOD];
	time_t FLastTick;

public:
	TPerformanceCounter();
	~TPerformanceCounter();

	void reset();

	void touch();
	void touch(unsigned AValue);

	unsigned current() const;
	TValue average() const;
};

template<typename TValue, const unsigned PERIOD>
inline TPerformanceCounter<TValue, PERIOD>::TPerformanceCounter() {
	reset();
}

template<typename TValue, const unsigned PERIOD>
inline TPerformanceCounter<TValue, PERIOD>::~TPerformanceCounter() {
}

template<typename TValue, const unsigned PERIOD>
inline void TPerformanceCounter<TValue, PERIOD>::reset() {
	memset(FCounter, 0, sizeof(FCounter));
	FLastTick = 0;
}

template<typename TValue, const unsigned PERIOD>
inline void TPerformanceCounter<TValue, PERIOD>::touch() {
	touch(1);
}

template<typename TValue, const unsigned PERIOD>
inline void TPerformanceCounter<TValue, PERIOD>::touch(unsigned AValue) {
	const int size = sizeof(FCounter) / sizeof(*FCounter);
	const time_t now = time(NULL);
	const unsigned i = now % size;

	if (FLastTick == now) {
		FCounter[i] += AValue;
		return;
	}

	const unsigned diff = now - FLastTick;

	if (diff - 1 == 0) {
		FCounter[i] = AValue;
		++FLastTick;
		return;
	}

	if (diff - PERIOD > 0) {
		memset(FCounter, 0, sizeof(FCounter));
	} else {
		unsigned i0 = FLastTick % size;
		if (i0 < i) {
			for (unsigned k = i0 + 1; k < i; ++k)
				FCounter[k] = 0;
		} else {
			while (++i0 < PERIOD)
				FCounter[i0] = 0;

			for (i0 = 0; i0 < i; ++i0)
				FCounter[i0] = 0;
		}
	}

	FCounter[i] = 1;
	FLastTick = now;
}

template<typename TValue, const unsigned PERIOD>
inline unsigned TPerformanceCounter<TValue, PERIOD>::current() const {
	const unsigned prev = FLastTick > 0 ? FLastTick - 1: PERIOD - 1;
	return FCounter[prev % PERIOD];
}

template<typename TValue, const unsigned PERIOD>
inline TValue TPerformanceCounter<TValue, PERIOD>::average() const {
	TValue avg(0);

	for (const unsigned *counter = FCounter, *end = counter + PERIOD; counter != end; ++counter)
		avg += *counter;

	avg /= TValue(PERIOD);

	return avg;
}
// }}}

// {{{ progress statistics
// fps and write bps counter for an average period of 10 seconds
TPerformanceCounter<double, 10> FFpsCounter;
TPerformanceCounter<double, 10> FBpsCounter;

const char *getElapsed() {
	static time_t first = time(0);
	static char buf[64];

	struct tm tm;
	time_t now = time(0);
	time_t diff = now - first;

	strftime(buf, sizeof(buf), "%T", gmtime_r(&diff, &tm));

	return buf;
}

const char *getEta() {
	return "--:--:--"; //! \todo return the ETA until finished.
}

const char *getProgress() {
//	"%02.2f%%"
	return "--.--%"; //! \todo return the percentage of how much we processed already
}

void printProcess() {
	static time_t last = 0;
	time_t now = time(0);

	if (now == last || !verbose)
		return;

	last = now;

	fprintf(stderr, 
		"\rfps: %3d, ~%3.3f | Mbps: %3.3f, ~%3.3f | elapsed: %s | ETA: %s | Progress: %s  ",
		FFpsCounter.current(), FFpsCounter.average(),
		FBpsCounter.current() / double(1024 * 1024), FBpsCounter.average() / double(1024 * 1024),
		getElapsed(),
		getEta(),
		getProgress()
	);
}
// }}}

void parseCmdLineArgs(int argc, char *argv[]) {//{{{
	int nargs = 1;
	for (int c; (c = getopt(argc, argv, "r:i:c:o:hq")) != -1; ++nargs) {
		switch (c) {
			case 'q':
				verbose = 0;
				break;
			case 'r':
				fps = atoi(optarg);
				break;
			case 'i':
				if (strcmp(optarg, "-") == 0)
					inputFd = STDIN_FILENO;
				else if ((inputFd = open(optarg, O_RDONLY | O_LARGEFILE)) == -1)
					die("Error opening input file(%s): %s", optarg, strerror(errno));

				break;
			case 'o':
				if (strcmp(optarg, "-") == 0)
					outputFd = STDOUT_FILENO;
				else if ((outputFd = open(optarg, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE, 0644)) == -1)
					die("Error opening output file(%s): %s", optarg, strerror(errno));

				break;
			case 'c':
#if THEORA
				if (strcmp(optarg, "theora") == 0) {
					encoder = new TOggTheoraEncoder();
					break;
				}
#endif
				if (strcmp(optarg, "y4m") == 0) {
					encoder = new TY4MEncoder();
					break;
				}
				die("Unsupported output codec: %s\n", optarg);
			case 'h':
				printHelp();
				exit(0);
			default:
				break;
		}
	}

	if (!encoder)
		encoder = new TY4MEncoder(); // default to y4m

	if (inputFd == -1)
		die("No input file specified");

	if (outputFd == -1)
		die("No output file specified");

	info.format = CAPSEO_FORMAT_YUV420;
	if (int error = CapseoStreamCreateFd(CAPSEO_MODE_DECODE, &info, inputFd, &stream))
		die("Could not create input stream (error %d)", error);
}//}}}

int main(int argc, char *argv[]) {
	bzero(&info, sizeof(capseo_info_t));

	parseCmdLineArgs(argc, argv);

	encoder->initialize();

	capseo_frame_t *frames[2];

	CapseoStreamDecodeFrame(stream, &frames[0], true);
	CapseoStreamDecodeFrame(stream, &frames[1], true);

	uint64_t timeStep = 1000000 / fps;
	uint64_t timeNext = frames[0]->id;

	for (;;) {
		while (diff(frames[0]->id, timeNext) > diff(frames[1]->id, timeNext)) {
			frames[0] = frames[1];
			FDupFrame = false;

			if (int error = CapseoStreamDecodeFrame(stream, &frames[1], true)) {
				if (error == CAPSEO_STREAM_END)
					goto out;

				return die("CapseoStreamDecodeFrame: decode error (code %d)", error);
			}

			if (!frames[1])
				goto out;
		}

		FFpsCounter.touch();
		unsigned nwritten = encoder->writeFrame(frames[0]);
		FBpsCounter.touch(nwritten);

		FDupFrame = true;
		timeNext += timeStep;

		printProcess();
	}

out:
	// write last frame
	unsigned nwritten = encoder->writeFrame(frames[0], true);
	FBpsCounter.touch(nwritten);
	FFpsCounter.touch();

	encoder->finalize();
	delete encoder;

	CapseoStreamDestroy(stream);

	printProcess();
	printf("\n");

	return 0;
}
