/*
 *  ummd ( Micro MultiMedia Daemon )
 *
 *  Copyright (C) 2010 Nicolas Thill <nicolas.thill@gmail.com>
 */

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "util/log.h"
#include "util/rbuf.h"

#include <fcntl.h>
#include <unistd.h>

#include <libavformat/avformat.h>
#include <linux/soundcard.h>

static AVOutputFormat *my_guess_format(const char *short_name, const char *filename, const char *mime_type)
{
#if LIBAVFORMAT_VERSION_MAJOR < 53 && LIBAVFORMAT_VERSION_MINOR < 45
	return guess_format(short_name, filename, mime_type);
#else
	return = av_guess_format(short_name, filename, mime_type);
#endif
}

typedef struct my_source_s my_source_t;
struct my_source_s
{
	char *name;
	int fd;
	my_rbuf_t *rb;
	uint8_t io_buf[AVCODEC_MAX_AUDIO_FRAME_SIZE];
	ByteIOContext *ff_io;
	AVFormatContext *ff_fc;
	AVCodecContext *ff_cc;
	int stream_index;
	int eof;
	int64_t pos;
};

typedef struct my_target_s my_target_t;
struct my_target_s
{
	char *name;
	int fd;
	my_rbuf_t *rb;
	uint8_t io_buf[AVCODEC_MAX_AUDIO_FRAME_SIZE];
	ByteIOContext *ff_io;
	AVFormatContext *ff_fc;
	AVCodecContext *ff_cc;
	int64_t pos;
};

uint8_t buf[AVCODEC_MAX_AUDIO_FRAME_SIZE];


static char *me;

static my_source_t my_source;
static my_target_t my_target;

#define MY_SOURCE_SIZE AVCODEC_MAX_AUDIO_FRAME_SIZE * 10
#define MY_TARGET_SIZE AVCODEC_MAX_AUDIO_FRAME_SIZE * 10

#define MY_RBLOCK_SIZE 1024
#define MY_WBLOCK_SIZE 1024

static char *my_get_seek_str(int whence)
{
	switch (whence) {
		case AVSEEK_SIZE:
			return "AVSEEK_SIZE";
		case SEEK_SET:
			return "AVSEEK_SET";
		case SEEK_CUR:
			return "AVSEEK_CUR";
		case SEEK_END:
			return "AVSEEK_END";
	}
	return "UNKNOWN";
}

static int my_source_read(void *opaque, uint8_t *buf, int buf_size)
{
	int n;

	n = my_rbuf_get(my_source.rb, buf, buf_size);
	my_source.pos += n;

	my_log(MY_LOG_NOTICE, "source: read(..., ..., %d) called, result = %d", buf_size, n);

	return n;
}

static int64_t my_source_seek(void *opaque, int64_t offset, int whence)
{
	int64_t pos;

	if (whence == SEEK_CUR) {
		pos = my_source.pos;
	} else {
		pos = -1;
	}

	my_log(MY_LOG_NOTICE, "source: seek(..., %lld, %s) called, result = %lld", offset, my_get_seek_str(whence), pos);

	return pos;
}

static int my_source_open(char *name)
{
	my_log(MY_LOG_NOTICE, "source: opening '%s'", name);
	my_source.fd = open(name, O_RDONLY);
	if (my_source.fd == -1) {
		my_log(MY_LOG_ERROR, "source: opening '%s'", name);
		goto _MY_ERR_open;
	}

	my_log(MY_LOG_NOTICE, "source: creating ring buffer");
	my_source.rb = my_rbuf_create(MY_SOURCE_SIZE);
	if (my_source.rb == NULL) {
		my_log(MY_LOG_ERROR, "source: creating ring buffer");
		goto _MY_ERR_rbuf_create;
	}

	my_log(MY_LOG_NOTICE, "source: creating I/O buffer");
	my_source.ff_io = av_alloc_put_byte(my_source.io_buf, sizeof(my_source.io_buf), 0, &my_source, my_source_read, NULL, my_source_seek);
	if (my_source.ff_io == NULL) {
		my_log(MY_LOG_ERROR, "source: creating I/O buffer");
		goto _MY_ERR_av_alloc_put_byte;
	}
	my_source.ff_io->is_streamed = 1;

	my_source.name = name;

	return 0;


_MY_ERR_av_alloc_put_byte:
_MY_ERR_rbuf_create:
	close(my_source.fd);
_MY_ERR_open:
	return -1;
}

static int my_source_init(void)
{
	AVCodec *av_dec;
	AVFormatParameters av_fp;
	AVInputFormat *av_if;
	AVProbeData av_pd;
	int i, n, rc;

/*
 * MP3: needs 2
 * WAV: needs 3
 */
#define MY_PROBE_MIN  AVCODEC_MAX_AUDIO_FRAME_SIZE * 3
	n = my_rbuf_get_avail(my_source.rb);
	if (!my_source.eof && n < MY_PROBE_MIN) {
		return -1;
	}

	n = sizeof(buf) - AVPROBE_PADDING_SIZE;
	n = my_rbuf_peek(my_source.rb, buf, n);
	av_pd.buf = buf;
	av_pd.buf_size = n;
	av_pd.filename = NULL;

	memset(&av_fp, 0, sizeof(av_fp));

	my_log(MY_LOG_NOTICE, "source: probing format");
	av_if = av_probe_input_format(&av_pd, 1);
	if (av_if == NULL) {
		av_if = av_find_input_format("s16le");
		av_fp.sample_rate = 44000;
		av_fp.channels = 2;
	}
	if (av_if == NULL) {
		my_log(MY_LOG_ERROR, "source: probing format");
		goto _MY_ERR_av_probe_input_format;
	}

	my_log(MY_LOG_NOTICE, "source: format found: %s", av_if->name);

	my_source.pos = 0;

	my_log(MY_LOG_NOTICE, "source: opening stream");
	rc = av_open_input_stream(&(my_source.ff_fc), my_source.ff_io, "", av_if, &av_fp);
	if( rc < 0 ) {
		my_log(MY_LOG_ERROR, "source: opening stream (rc = %d)", rc);
		goto _MY_ERR_av_open_input_stream;
	}

	my_log(MY_LOG_NOTICE, "source: finding stream info");
	rc = av_find_stream_info(my_source.ff_fc);
	if( rc < 0 ) {
		my_log(MY_LOG_ERROR, "source: finding stream info (rc = %d)", rc);
		goto _MY_ERR_av_find_stream_info;
	}

	my_log(MY_LOG_NOTICE, "source: finding audio stream");
	my_source.stream_index = -1;
	for (i = 0; i < my_source.ff_fc->nb_streams; i++) {
		if (my_source.ff_fc->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO && my_source.stream_index < 0) {
			my_source.stream_index = i;
			break;
		}
	}
	if (my_source.stream_index < 0) {
		my_log(MY_LOG_ERROR, "source: finding audio stream");
		goto _MY_ERR_finding_audio_stream;
	}

	my_source.ff_cc = my_source.ff_fc->streams[my_source.stream_index]->codec;

	my_log(MY_LOG_NOTICE, "source: finding audio codec");
	av_dec = avcodec_find_decoder(my_source.ff_cc->codec_id);
	if (!av_dec) {
		my_log(MY_LOG_NOTICE, "source: finding audio codec");
		goto _MY_ERR_avcodec_find_decoder;
	}

	my_log(MY_LOG_NOTICE, "source: audio stream found, codec: %s, channels: %d, sample-rate: %d Hz", av_dec->name, my_source.ff_cc->channels, my_source.ff_cc->sample_rate);

	my_log(MY_LOG_NOTICE, "source: opening audio codec");
	rc = avcodec_open(my_source.ff_cc, av_dec);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "source: opening audio codec (rc = %d)", rc);
		goto _MY_ERR_avcodec_open;
	}

	return 0;

_MY_ERR_avcodec_open:
_MY_ERR_avcodec_find_decoder:
_MY_ERR_finding_audio_stream:
_MY_ERR_av_find_stream_info:
_MY_ERR_av_open_input_stream:
_MY_ERR_av_probe_input_format:
	return -1;
}


static int my_target_write(void *opaque, uint8_t *buf, int buf_size)
{
	int n;

	my_log(MY_LOG_NOTICE, "target: write(..., ..., %d) called", buf_size);

	n = my_rbuf_put(my_target.rb, buf, buf_size);
	my_target.pos += n;

	return n;
}

static int64_t my_target_seek(void *opaque, int64_t offset, int whence)
{
	my_log(MY_LOG_NOTICE, "target: seek(..., %lld, %d) called", offset, whence);
	if (whence == AVSEEK_SIZE) {
		my_log(MY_LOG_NOTICE, "target: seek: AVSEEK_SIZE");
		return -1;
	}
	if (whence == SEEK_SET) {
		my_log(MY_LOG_NOTICE, "target: seek: AVSEEK_SET");
	}
	if (whence == SEEK_CUR) {
		my_log(MY_LOG_NOTICE, "target: seek: AVSEEK_CUR");
	}
	if (whence == SEEK_END) {
		my_log(MY_LOG_NOTICE, "target: seek: AVSEEK_END");
	}
	my_log(MY_LOG_NOTICE, "target: seek: pos=%lld", my_target.pos);

	return my_target.pos;
}

static int my_target_open(char *name)
{
	my_log(MY_LOG_NOTICE, "target: opening '%s'", name);
	my_target.fd = open(name, O_WRONLY);
	if (my_target.fd == -1) {
		my_log(MY_LOG_ERROR, "target: opening '%s'", name);
		goto _MY_ERR_open;
	}

	my_log(MY_LOG_NOTICE, "target: creating ring buffer");
	my_target.rb = my_rbuf_create(MY_TARGET_SIZE);
	if (my_target.rb == NULL) {
		my_log(MY_LOG_ERROR, "target: creating ring buffer");
		goto _MY_ERR_rbuf_create;
	}

	my_log(MY_LOG_NOTICE, "target: creating I/O buffer");
	my_target.ff_io = av_alloc_put_byte(my_target.io_buf, sizeof(my_target.io_buf), 1, &my_target, NULL, my_target_write, my_target_seek);
	if (my_target.ff_io == NULL) {
		my_log(MY_LOG_ERROR, "target: creating I/O buffer");
		goto _MY_ERR_av_alloc_put_byte;
	}
	my_target.ff_io->is_streamed = 1;

	my_target.name = name;

	return 0;


_MY_ERR_av_alloc_put_byte:
_MY_ERR_rbuf_create:
	close(my_target.fd);
_MY_ERR_open:
	return -1;
}

static int my_target_init(void)
{
	AVCodec *av_enc;
	AVOutputFormat *av_of;
	AVFormatContext *av_fc;
	AVFormatParameters av_fp;
	AVStream *av_st;
	int i, rc;

	i = my_source.ff_cc->channels;
	rc = ioctl(my_target.fd, SNDCTL_DSP_CHANNELS, &i);
	if (rc == -1) {
		my_log(MY_LOG_ERROR, "target: setting channels for output device (%d: %s)", errno, strerror(errno));
	}

	i = my_source.ff_cc->sample_rate;
	rc = ioctl(my_target.fd, SNDCTL_DSP_SPEED, &i);
	if (rc == -1) {
		my_log(MY_LOG_ERROR, "target: setting sample rate for output device (%d: %s)", errno, strerror(errno));
	}

	i = AFMT_S16_NE;
	rc = ioctl(my_target.fd, SNDCTL_DSP_SETFMT, &i);
	if (rc == -1) {
		my_log(MY_LOG_ERROR, "target: setting audio format for output device (%d: %s)", errno, strerror(errno));
	}

	my_log(MY_LOG_NOTICE, "target: guessing format");
	av_of = my_guess_format(NULL, my_source.name, NULL);
	if (av_of == NULL) {
		av_of = my_guess_format("s16le", NULL, NULL);
	}
	if (av_of == NULL) {
		my_log(MY_LOG_ERROR, "target: guessing format");
		goto _MY_ERR_av_guess_format;
	}

	my_log(MY_LOG_NOTICE, "target: format found: %s", av_of->name);

	my_target.pos = 0;

	my_log(MY_LOG_NOTICE, "target: creating format context");
	av_fc = avformat_alloc_context();
	if( rc < 0 ) {
		my_log(MY_LOG_ERROR, "target: creating format context");
		goto _MY_ERR_avformat_alloc_context;
	}
	my_target.ff_fc = av_fc;
	my_target.ff_fc->oformat = av_of;

	my_log(MY_LOG_NOTICE, "target: setting parameters");
	rc = av_set_parameters(my_target.ff_fc, &av_fp);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "target: setting format parameters");
		goto _MY_ERR_av_set_parameters;
	}

	my_log(MY_LOG_NOTICE, "target: creating audio stream");
	av_st = av_new_stream(my_target.ff_fc, 1);
	if (av_st == NULL) {
		my_log(MY_LOG_ERROR, "target: creating audio stream");
		goto _MY_ERR_av_new_stream;
	}
	
	my_target.ff_cc = av_st->codec;
	my_target.ff_cc->codec_id = CODEC_ID_PCM_S16LE;
	my_target.ff_cc->codec_type = CODEC_TYPE_AUDIO;
	my_target.ff_cc->sample_rate = 44100;
	my_target.ff_cc->channels = 2;

	my_log(MY_LOG_NOTICE, "target: finding audio codec");
	av_enc = avcodec_find_decoder(my_target.ff_cc->codec_id);
	if (!av_enc) {
		my_log(MY_LOG_NOTICE, "target: finding source codec");
		goto _MY_ERR_avcodec_find_decoder;
	}

	my_log(MY_LOG_NOTICE, "target: audio stream created, codec: %s, channels: %d, sample-rate: %d Hz", av_enc->name, my_target.ff_cc->channels, my_target.ff_cc->sample_rate);

	my_log(MY_LOG_NOTICE, "target: opening audio codec");
	if (avcodec_open(my_target.ff_cc, av_enc) < 0) {
		my_log(MY_LOG_ERROR, "target: opening audio codec");
		goto _MY_ERR_avcodec_open;
	}

	return 0;

_MY_ERR_avcodec_open:
_MY_ERR_avcodec_find_decoder:
_MY_ERR_av_new_stream:
_MY_ERR_av_set_parameters:
_MY_ERR_avformat_alloc_context:
_MY_ERR_av_guess_format:
_MY_ERR_open:
	return -1;
}

static int running = 0;

static int my_loop(void)
{
	AVPacket av_pk;
	int i;
	int target_size;
	int source_size;
	int nfds;
	fd_set rfds, wfds;
	struct timeval tv;
	int n;
	int init_done = 0;

	running = 1;
	while (running) {
		nfds = 0;
		FD_ZERO(&rfds);
		FD_SET(my_source.fd, &rfds);
		if (my_source.fd > nfds) {
			nfds = my_source.fd;
		}
		FD_ZERO(&wfds);
		FD_SET(my_target.fd, &wfds);
		if (my_target.fd > nfds) {
			nfds = my_target.fd;
		}
		tv.tv_sec = 0;
		tv.tv_usec = 100 * 1000;
		n = select(nfds + 1, &rfds, &wfds, NULL, &tv);
		if (n < 0) {
			if (errno != EINTR) {
				my_log(MY_LOG_ERROR, "select '%d: %s'", errno, strerror(errno));
				goto _MY_ERR_select;
			}
		} else if (n > 0) {
			if (FD_ISSET(my_source.fd, &rfds)) {
				n = my_rbuf_put_avail(my_source.rb);
				if (n > MY_RBLOCK_SIZE) {
					n = MY_RBLOCK_SIZE;
				}
				n = read(my_source.fd, buf, n);
/*
				my_log(MY_LOG_NOTICE, "source: read %d bytes", n);
*/
				if (n == -1) {
					if (errno != EINTR && errno != EAGAIN) {
						my_log(MY_LOG_ERROR, "read '%d: %s'", errno, strerror(errno));
						goto _MY_ERR_read;
					}
				} else if (n == 0) {
					my_source.eof = 1;
				} else {
					n = my_rbuf_put(my_source.rb, buf, n);
/*
					my_log(MY_LOG_NOTICE, "source: put %d bytes in ring buffer", n);
*/
				}
			}
			if (FD_ISSET(my_target.fd, &wfds)) {
				n = MY_WBLOCK_SIZE;
				n = my_rbuf_get(my_target.rb, buf, n);
/*
				my_log(MY_LOG_NOTICE, "target: got %d bytes from ring buffer", n);
*/
				if (n > 0) {
					n = write(my_target.fd, buf, n);
/*
					my_log(MY_LOG_NOTICE, "target: wrote %d bytes", n);
*/
					if (n == -1) {
						if (errno != EINTR && errno != EAGAIN) {
							my_log(MY_LOG_ERROR, "write '%d: %s'", errno, strerror(errno));
							goto _MY_ERR_write;
						}
					}
				}
			}
		} else {
			; /* time out */
		}

		if (init_done) {
			if (!my_source.eof && my_rbuf_get_avail(my_source.rb) < AVCODEC_MAX_AUDIO_FRAME_SIZE) {
				continue;
			}
			if (my_rbuf_put_avail(my_target.rb) < AVCODEC_MAX_AUDIO_FRAME_SIZE) {
				continue;
			}
			if (av_read_frame(my_source.ff_fc, &av_pk) == 0) {
				if (av_pk.stream_index == my_source.stream_index) {
					my_log(MY_LOG_NOTICE, "source: read an audio frame, pts: %lld, dts: %lld, size: %d, duration: %d, pos: %lld", av_pk.pts, av_pk.dts, av_pk.size, av_pk.duration, av_pk.pos);
					target_size = sizeof(buf);
					source_size = avcodec_decode_audio2(my_source.ff_cc, (int16_t *)buf, &target_size, av_pk.data, av_pk.size);
					av_free_packet(&av_pk);
					if (source_size < 0) {
						my_log(MY_LOG_ERROR, "source: decoding audio frame");
					}
					my_log(MY_LOG_NOTICE, "source: decoded an audio frame, source_size: %d, target_size: %d", source_size, target_size);
					i = my_rbuf_put(my_target.rb, buf, target_size);
					if (i < 0) {
						my_log(MY_LOG_ERROR, "target: writing audio frame");
					}
					my_log(MY_LOG_NOTICE, "target: wrote an audio frame, size: %d, target_size: %d", i, target_size);
				}
			}
		} else {
			if (my_source_init() == 0) {
				my_target_init();
				init_done = 1;
				my_log(MY_LOG_NOTICE, "init: done");
			}
		}
	}

	return 0;

_MY_ERR_write:
_MY_ERR_read:
_MY_ERR_select:
	return -1;
}


int main(int argc, char **argv)
{
	me = strrchr(argv[0], '/');
	if (me == NULL ) {
		me = argv[0];
	} else {
		me++;
	}

	my_log_init(me);
	
	av_register_all();
	avdevice_register_all();

	if (my_log_open("stderr", MY_LOG_DEBUG) != 0) {
		goto _MY_ERR_log_open;
	}

	if (argc < 2) {
		my_log(MY_LOG_ERROR, "not enough arguments");
		goto _MY_ERR_argc;
	}

	if (my_source_open(argv[1]) < 0) {
		goto _MY_ERR_open_source;
	}

	if (my_target_open(argv[2]) < 0) {
		goto _MY_ERR_open_target;
	}

	my_loop();

	my_log_close();

	return 0;

_MY_ERR_open_target:
_MY_ERR_open_source:
_MY_ERR_argc:
	my_log_close();
_MY_ERR_log_open:
	return 1;
}
