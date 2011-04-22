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

typedef struct my_dec_s my_dec_t;
struct my_dec_s
{
	uint8_t *io_buffer;
	int io_buffer_size;
	ByteIOContext *ff_io;
	AVInputFormat  *ff_if;
	AVFormatContext *ff_fc;
	AVCodecContext *ff_cc;
	int64_t pos;
};

typedef struct my_enc_s my_enc_t;
struct my_enc_s
{
	uint8_t *io_buffer;
	int io_buffer_size;
	ByteIOContext *ff_io;
	AVOutputFormat  *ff_of;
	AVFormatContext *ff_fc;
	AVCodecContext *ff_cc;
	int64_t pos;
};


static char *me;
static my_dec_t my_dec;
static my_enc_t my_enc;
static int source_fd, target_fd;

char source_buf[AVCODEC_MAX_AUDIO_FRAME_SIZE];
char target_buf[AVCODEC_MAX_AUDIO_FRAME_SIZE];

int source_stream_index;

static int my_source_read(void *opaque, uint8_t *buf, int buf_size)
{
	my_dec_t *d = opaque;

	int n;
	my_log(MY_LOG_NOTICE, "source: read(..., ..., %d) called", buf_size);
	n = read(source_fd, buf, buf_size);
	d->pos += n;
	return n;
}

static int64_t my_source_seek(void *opaque, int64_t offset, int whence)
{
	my_dec_t *d = opaque;

	my_log(MY_LOG_NOTICE, "source: seek(..., %lld, %d) called", offset, whence);
	if (whence == AVSEEK_SIZE) {
		my_log(MY_LOG_NOTICE, "source: seek: AVSEEK_SIZE");
		return -1;
	}
	if (whence == SEEK_SET) {
		my_log(MY_LOG_NOTICE, "source: seek: AVSEEK_SET");
	}
	if (whence == SEEK_CUR) {
		my_log(MY_LOG_NOTICE, "source: seek: AVSEEK_CUR");
	}
	if (whence == SEEK_END) {
		my_log(MY_LOG_NOTICE, "source: seek: AVSEEK_END");
	}
	my_log(MY_LOG_NOTICE, "source: seek: pos=%lld", d->pos);
	return d->pos;
}

static int my_source_open(char *name)
{
	AVCodec *av_dec;
	AVFormatParameters av_fp;
	AVInputFormat *av_if;
	AVProbeData av_pd;
	int i, rc;

	my_log(MY_LOG_NOTICE, "source: opening '%s'", name);
	source_fd = open(name, O_RDONLY);
	if (source_fd == -1) {
		my_log(MY_LOG_ERROR, "source: opening '%s'", name);
		goto _MY_ERR_open;
	}

	bzero(source_buf, sizeof(source_buf));

	rc = read(source_fd, source_buf, sizeof(source_buf));

	av_pd.buf = source_buf;
	av_pd.buf_size = sizeof(source_buf);
	av_pd.filename = NULL;

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
	my_dec.ff_if = av_if;

	my_log(MY_LOG_NOTICE, "source: format found: %s", av_if->name);

	lseek(source_fd, 0, SEEK_SET);

	my_dec.io_buffer = source_buf;
	my_dec.io_buffer_size = sizeof(source_buf);

	my_log(MY_LOG_NOTICE, "source: creating I/O buffer");
	my_dec.ff_io = av_alloc_put_byte(my_dec.io_buffer, my_dec.io_buffer_size, 0, &my_dec, my_source_read, NULL, my_source_seek);
	if (my_dec.ff_io == NULL) {
		my_log(MY_LOG_ERROR, "source: creating I/O buffer");
		goto _MY_ERR_av_alloc_put_byte;
	}

	my_dec.ff_io->is_streamed = 1;
	my_dec.pos = 0;

	my_log(MY_LOG_NOTICE, "source: opening stream");
	rc = av_open_input_stream(&(my_dec.ff_fc), my_dec.ff_io, "", my_dec.ff_if, &av_fp);
	if( rc < 0 ) {
		my_log(MY_LOG_ERROR, "source: opening stream");
		goto _MY_ERR_av_open_input_stream;
	}

	my_log(MY_LOG_NOTICE, "source: finding stream info");
	rc = av_find_stream_info(my_dec.ff_fc);
	if( rc < 0 ) {
		my_log(MY_LOG_ERROR, "source: finding stream info");
		goto _MY_ERR_av_find_stream_info;
	}

	my_log(MY_LOG_NOTICE, "source: finding audio stream");
	source_stream_index = -1;
	for (i = 0; i < my_dec.ff_fc->nb_streams; i++) {
		if (my_dec.ff_fc->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO && source_stream_index < 0) {
			source_stream_index = i;
			break;
		}
	}
	if (source_stream_index < 0) {
		my_log(MY_LOG_ERROR, "source: finding audio stream");
		goto _MY_ERR_finding_audio_stream;
	}

	my_dec.ff_cc = my_dec.ff_fc->streams[source_stream_index]->codec;

	my_log(MY_LOG_NOTICE, "source: finding audio codec");
	av_dec = avcodec_find_decoder(my_dec.ff_cc->codec_id);
	if (!av_dec) {
		my_log(MY_LOG_NOTICE, "source: finding audio codec");
		goto _MY_ERR_avcodec_find_decoder;
	}

	my_log(MY_LOG_NOTICE, "source: audio stream found, codec: %s, channels: %d, sample-rate: %d Hz", av_dec->name, my_dec.ff_cc->channels, my_dec.ff_cc->sample_rate);

	my_log(MY_LOG_NOTICE, "source: opening audio codec");
	if (avcodec_open(my_dec.ff_cc, av_dec) < 0) {
		my_log(MY_LOG_ERROR, "source: opening audio codec");
		goto _MY_ERR_avcodec_open;
	}

	return 0;

_MY_ERR_avcodec_open:
_MY_ERR_avcodec_find_decoder:
_MY_ERR_finding_audio_stream:
_MY_ERR_av_find_stream_info:
_MY_ERR_av_open_input_stream:
_MY_ERR_av_alloc_put_byte:
_MY_ERR_av_probe_input_format:
_MY_ERR_open:
	return -1;
}


static int my_target_write(void *opaque, uint8_t *buf, int buf_size)
{
	my_enc_t *d = opaque;

	int n;
	my_log(MY_LOG_NOTICE, "target: write(..., ..., %d) called", buf_size);
	n = write(source_fd, buf, buf_size);
	d->pos += n;
	return n;
}

static int64_t my_target_seek(void *opaque, int64_t offset, int whence)
{
	my_dec_t *d = opaque;

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
	my_log(MY_LOG_NOTICE, "target: seek: pos=%lld", d->pos);
	return d->pos;
}

static int my_target_open(char *name)
{
	AVCodec *av_enc;
	AVOutputFormat *av_of;
	AVFormatContext *av_fc;
	AVFormatParameters av_fp;
	AVStream *av_st;
	int i, rc;

	my_log(MY_LOG_NOTICE, "target: opening '%s'", name);
	target_fd = open(name, O_WRONLY);
	if (target_fd == -1) {
		my_log(MY_LOG_ERROR, "target: opening '%s'", name);
		goto _MY_ERR_open;
	}

	i = my_dec.ff_cc->channels;
	rc = ioctl(target_fd, SNDCTL_DSP_CHANNELS, &i);
	if (rc == -1) {
		my_log(MY_LOG_ERROR, "target: setting channels for output device (%d: %s)", errno, strerror(errno));
	}

	i = my_dec.ff_cc->sample_rate;
	rc = ioctl(target_fd, SNDCTL_DSP_SPEED, &i);
	if (rc == -1) {
		my_log(MY_LOG_ERROR, "target: setting sample rate for output device (%d: %s)", errno, strerror(errno));
	}

	i = AFMT_S16_NE;
	rc = ioctl(target_fd, SNDCTL_DSP_SETFMT, &i);
	if (rc == -1) {
		my_log(MY_LOG_ERROR, "target: setting audio format for output device (%d: %s)", errno, strerror(errno));
	}

	my_log(MY_LOG_NOTICE, "target: guessing format");
	av_of = my_guess_format(NULL, name, NULL);
	if (av_of == NULL) {
		av_of = my_guess_format("s16le", NULL, NULL);
	}
	if (av_of == NULL) {
		my_log(MY_LOG_ERROR, "target: guessing format");
		goto _MY_ERR_av_guess_format;
	}
	my_enc.ff_of = av_of;

	my_log(MY_LOG_NOTICE, "target: format found: %s", av_of->name);

	my_enc.io_buffer = target_buf;
	my_enc.io_buffer_size = sizeof(target_buf);

	my_log(MY_LOG_NOTICE, "target: creating I/O buffer");
	my_enc.ff_io = av_alloc_put_byte(my_enc.io_buffer, my_enc.io_buffer_size, 1, &my_enc, NULL, my_target_write, my_target_seek);
	if (my_enc.ff_io == NULL) {
		my_log(MY_LOG_ERROR, "target: creating I/O buffer");
		goto _MY_ERR_av_alloc_put_byte;
	}

	my_enc.ff_io->is_streamed = 1;
	my_enc.pos = 0;

	my_log(MY_LOG_NOTICE, "target: creating format context");
	av_fc = avformat_alloc_context();
	if( rc < 0 ) {
		my_log(MY_LOG_ERROR, "target: creating format context");
		goto _MY_ERR_avformat_alloc_context;
	}
	my_enc.ff_fc = av_fc;
	my_enc.ff_fc->oformat = my_enc.ff_of;

	my_log(MY_LOG_NOTICE, "target: setting parameters");
	rc = av_set_parameters(my_enc.ff_fc, &av_fp);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "target: setting format parameters");
		goto _MY_ERR_av_set_parameters;
	}

	my_log(MY_LOG_NOTICE, "target: creating audio stream");
	av_st = av_new_stream(my_enc.ff_fc, 1);
	if (av_st == NULL) {
		my_log(MY_LOG_ERROR, "target: creating audio stream");
		goto _MY_ERR_av_new_stream;
	}
	
	my_enc.ff_cc = av_st->codec;
	my_enc.ff_cc->codec_id = CODEC_ID_PCM_S16LE;
	my_enc.ff_cc->codec_type = CODEC_TYPE_AUDIO;
	my_enc.ff_cc->sample_rate = 44100;
	my_enc.ff_cc->channels = 2;

	my_log(MY_LOG_NOTICE, "target: finding audio codec");
	av_enc = avcodec_find_decoder(my_enc.ff_cc->codec_id);
	if (!av_enc) {
		my_log(MY_LOG_NOTICE, "target: finding source codec");
		goto _MY_ERR_avcodec_find_decoder;
	}

	my_log(MY_LOG_NOTICE, "target: audio stream created, codec: %s, channels: %d, sample-rate: %d Hz", av_enc->name, my_enc.ff_cc->channels, my_enc.ff_cc->sample_rate);

	my_log(MY_LOG_NOTICE, "target: opening audio codec");
	if (avcodec_open(my_enc.ff_cc, av_enc) < 0) {
		my_log(MY_LOG_ERROR, "target: opening audio codec");
		goto _MY_ERR_avcodec_open;
	}

	return 0;

_MY_ERR_avcodec_open:
_MY_ERR_avcodec_find_decoder:
_MY_ERR_av_new_stream:
_MY_ERR_av_set_parameters:
_MY_ERR_avformat_alloc_context:
_MY_ERR_av_alloc_put_byte:
_MY_ERR_av_guess_format:
_MY_ERR_open:
	return -1;
}

static int my_loop(void)
{
	AVPacket av_pk;
	int i;
	int target_size;
	int source_size;

	my_log(MY_LOG_NOTICE, "source: reading audio frames");
	while (av_read_frame(my_dec.ff_fc, &av_pk) >= 0) {
		if (av_pk.stream_index == source_stream_index) {
			my_log(MY_LOG_NOTICE, "source: read an audio frame, pts: %lld, dts: %lld, size: %d, duration: %d, pos: %lld", av_pk.pts, av_pk.dts, av_pk.size, av_pk.duration, av_pk.pos);
			target_size = sizeof(target_buf);
			source_size = avcodec_decode_audio2(my_dec.ff_cc, (int16_t *)target_buf, &target_size, av_pk.data, av_pk.size);
			av_free_packet(&av_pk);
			if (source_size < 0) {
				my_log(MY_LOG_ERROR, "source: decoding audio frame");
				goto _MY_ERR_avcodec_decode_audio2;
			}
			my_log(MY_LOG_NOTICE, "source: decoded an audio frame, source_size: %d, target_size: %d", source_size, target_size);
			i = write(target_fd, target_buf, target_size);
			if (i < 0) {
				my_log(MY_LOG_ERROR, "target: writing audio frame");
				goto _MY_ERR_avcodec_decode_audio2;
			}
			my_log(MY_LOG_NOTICE, "target: wrote an audio frame, size: %d, target_size: %d", i, target_size);
		}
	}
out:

	return 0;

_MY_ERR_avcodec_decode_audio2:
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
