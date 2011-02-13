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

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "util/audio.h"

#include "util/log.h"
#include "util/mem.h"

typedef struct my_audio_codec_priv_s my_audio_codec_priv_t;

typedef void (*my_audio_destroy_fn_t)(my_audio_codec_t *c);

typedef int (*my_audio_encode_fn_t)(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen);
typedef int (*my_audio_decode_fn_t)(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen);


struct my_audio_codec_priv_s {
	struct AVCodecContext *context;
	struct AVPacket packet;
	my_audio_destroy_fn_t destroy;
	my_audio_encode_fn_t encode;
	my_audio_decode_fn_t decode;
};

#define MY_AUDIO_CODEC_PRIV(p) ((my_audio_codec_priv_t *)(p))


/* private functions */

static my_audio_codec_priv_t *my_audio_codec_create_priv(void)
{
	my_audio_codec_priv_t *c;

	c = my_mem_alloc(sizeof(*c));
	if (!c) {
		my_log(MY_LOG_ERROR, "audio/codec: error allocating codec data (%d: %s)", errno, strerror(errno));
		goto _ERR_mem_alloc;
	}
	
	return c;

	my_mem_free(c);
_ERR_mem_alloc:
	return NULL;
}

static void my_mp3_codec_destroy_priv(my_audio_codec_t *c)
{
	my_mem_free(c);
}


/* raw encoder/decoder */

static my_audio_codec_priv_t *my_audio_codec_create_raw(void);
static void my_audio_codec_destroy_raw(my_audio_codec_t *c);
static int my_audio_codec_copy_raw(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen);

static my_audio_codec_priv_t *my_audio_codec_create_raw(void)
{
	my_audio_codec_priv_t *c;

	c = my_audio_codec_create_priv();
	if (!c) {
		goto _ERR_create_priv;
	}

	c->destroy = my_audio_codec_destroy_raw;
	c->encode = my_audio_codec_copy_raw;
	c->decode = my_audio_codec_copy_raw;

	return c;

	my_audio_codec_destroy_priv(c);
_ERR_create_priv:
	return NULL;
}

static void my_mp3_codec_destroy_raw(my_audio_codec_t *c)
{
	my_audio_codec_destroy_priv(c);
}

static int my_audio_codec_copy_raw(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen)
{
	int n = MIN(*ilen, *olen);

	memcpy(obuf, ibuf, n);
	*ilen = n;
	*olen = n;

	return n;
}


/* FFmpeg encoder/decoder */

static my_audio_codec_priv_t *my_audio_codec_create_ffmpeg(char *name);
static void my_audio_codec_destroy_ffmpeg(my_audio_codec_t *c);

static int my_audio_codec_decode_ffmpeg(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen);
static int my_audio_codec_encode_ffmpeg(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen);

static my_audio_codec_priv_t *my_audio_codec_create_ffmpeg(char *name)
{
	my_audio_codec_priv_t *c;
	AVCodec *codec;

	c = my_audio_codec_create_priv();
	if (!c) {
		goto _ERR_create_priv;
	}

	codec = avcodec_find_decoder_by_name(name);
	if (!codec) {
		my_log(MY_LOG_ERROR, "mp3: error finding codec (%s)", name);
		goto _ERR_avcodec_find_decoder;
	}

	c->context = avcodec_alloc_context();
	if (!c->context) {
		my_log(MY_LOG_ERROR, "mp3: error allocating context");
		goto _ERR_avcodec_alloc_context;
	}

	if (avcodec_open(c->context, codec) < 0) {
		my_log(MY_LOG_ERROR, "mp3: error opening decoder");
		goto _ERR_avcodec_open;
	}

	av_init_packet(&(c->packet));

	c->destroy = my_audio_codec_destroy_ffmpeg;
	c->encode = my_audio_codec_encode_ffmpeg;
	c->decode = my_audio_codec_decode_ffmpeg;

	return c;

	avcodec_close(c->context);
_ERR_avcodec_open:
	av_free(c->context);
_ERR_avcodec_alloc_context:
_ERR_avcodec_find_decoder:
	my_audio_codec_destroy_priv(c);
_ERR_create_priv:
	return NULL;
}

static void my_mp3_codec_destroy_ffmpeg(my_audio_codec_t *c)
{
	avcodec_close(MY_AUDIO_CODEC_PRIV(c)->context);
	av_free(MY_AUDIO_CODEC_PRIV(c)->context);
	my_mp3_codec_destroy_priv(c);
}

static int my_audio_codec_encode_ffmpeg(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen)
{
}

static int my_audio_codec_decode_ffmpeg(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen)
{
	int n;

	MY_AUDIO_CODEC_PRIV(c)->packet.data = ibuf;
	MY_AUDIO_CODEC_PRIV(c)->packet.size = *ilen - FF_INPUT_BUFFER_PADDING_SIZE;

	n = avcodec_decode_audio3(MY_AUDIO_CODEC_PRIV(c)->context, obuf, olen, &(MY_AUDIO_CODEC_PRIV(c)->packet));
	if (n < 0) {
		my_log(MY_LOG_ERROR, "mp3: error decoding audio");
		return n;
	}

	*ilen = n;

	return n;
}


/* public functions */

int my_audio_codec_init(void)
{
	avcodec_init();
	avcodec_register_all();

	return 0;
}

my_audio_codec_t *my_audio_codec_create(char *name)
{
	my_audio_codec_priv_t *c;

	if (name) {
		c = my_audio_codec_create_ffmpeg(name);
	} else {
		c = my_audio_codec_create_raw();
	}

	return (my_audio_codec_t *)c;
}

void my_audio_codec_destroy(my_audio_codec_t *c)
{
	MY_AUDIO_CODEC_PRIV(c)->destroy(c);
}

int my_audio_codec_encode(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen)
{
	MY_AUDIO_CODEC_PRIV(c)->encode(c, ibuf, ilen, obuf, olen);
}

int my_audio_codec_decode(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen)
{
	MY_AUDIO_CODEC_PRIV(c)->decode(c, ibuf, ilen, obuf, olen);
}
