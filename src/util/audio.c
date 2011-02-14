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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h> /* MIN/MAX */

#include <mpg123.h>

#include "util/audio.h"

#include "util/log.h"
#include "util/mem.h"

typedef struct my_audio_codec_priv_s my_audio_codec_priv_t;

typedef void (*my_audio_destroy_fn_t)(my_audio_codec_t *c);

typedef int (*my_audio_encode_fn_t)(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen);
typedef int (*my_audio_decode_fn_t)(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen);


struct my_audio_codec_priv_s {
	struct AVCodecContext *context;
	mpg123_handle *mpg123_h;
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

static void my_audio_codec_destroy_priv(my_audio_codec_t *c)
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

static void my_audio_codec_destroy_raw(my_audio_codec_t *c)
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


/* MPG123 encoder/decoder */

static my_audio_codec_priv_t *my_audio_codec_create_mpg123(void);
static void my_audio_codec_destroy_mpg123(my_audio_codec_t *c);
static int my_audio_codec_decode_mpg123(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen);

static my_audio_codec_priv_t *my_audio_codec_create_mpg123(void)
{
	my_audio_codec_priv_t *c;
	int rc;

	c = my_audio_codec_create_priv();
	if (!c) {
		goto _ERR_create_priv;
	}

	c->destroy = my_audio_codec_destroy_mpg123;
	c->encode = NULL;
	c->decode = my_audio_codec_decode_mpg123;

	c->mpg123_h = mpg123_new(NULL, &rc);
	if (c->mpg123_h == NULL) {
		my_log(MY_LOG_ERROR, "audio/codec: error creating MP3 codec data (%d: %s)", rc, mpg123_plain_strerror(rc));
		goto _ERR_mpg123_new;
	}

	rc = mpg123_format_none(c->mpg123_h);
	
	rc = mpg123_format(c->mpg123_h, 44100, MPG123_STEREO, MPG123_ENC_SIGNED_16);
	if (rc != MPG123_OK) {
		my_log(MY_LOG_ERROR, "audio/codec: error setting MP3 codec output format (%d: %s)", rc, mpg123_plain_strerror(rc));
		goto _ERR_mpg123_open_feed;
	}

	rc = mpg123_open_feed(c->mpg123_h);
	if (rc != MPG123_OK) {
		my_log(MY_LOG_ERROR, "audio/codec: error opening MP3 codec feed (%d: %s)", rc, mpg123_plain_strerror(rc));
		goto _ERR_mpg123_open_feed;
	}

	return c;

_ERR_mpg123_open_feed:
	mpg123_delete(c->mpg123_h);
_ERR_mpg123_new:
	my_audio_codec_destroy_priv(c);
_ERR_create_priv:
	return NULL;
}

static void my_audio_codec_destroy_mpg123(my_audio_codec_t *c)
{
	mpg123_delete(MY_AUDIO_CODEC_PRIV(c)->mpg123_h);
	my_audio_codec_destroy_priv(c);
}

static int my_audio_codec_decode_mpg123(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen)
{
	size_t n;
	int rc;
	long rate;
	int channels, enc;

	rc = mpg123_feed(MY_AUDIO_CODEC_PRIV(c)->mpg123_h, ibuf, *ilen);
	if (rc != MPG123_OK) {
		my_log(MY_LOG_ERROR, "audio/codec: error feeding MP3 (%d: %s)", rc, mpg123_plain_strerror(rc));
		goto _ERR_mpg123_feed;
	}

	rc = mpg123_read(MY_AUDIO_CODEC_PRIV(c)->mpg123_h, obuf, *olen, &n);
	if (rc == MPG123_NEW_FORMAT) {
		mpg123_getformat(MY_AUDIO_CODEC_PRIV(c)->mpg123_h, &rate, &channels, &enc);
		MY_DEBUG("audio/codec: found MP3 stream (rate: %li Hz, channels: %i, encoding: %08x)", rate, channels, enc);
	} else if ((rc != MPG123_OK) && (rc != MPG123_NEED_MORE)) {
		my_log(MY_LOG_ERROR, "audio/codec: error decoding MP3 (%d: %s)", rc, mpg123_plain_strerror(rc));
		goto _ERR_mpg123_read;
	}

	*olen = n;

	return n;

_ERR_mpg123_read:
_ERR_mpg123_feed:
	return 0;
}


/* public functions */

int my_audio_codec_init(void)
{
	mpg123_init();

	return 0;
}

void my_audio_codec_fini(void)
{
	mpg123_exit();
}

my_audio_codec_t *my_audio_codec_create(char *name)
{
	my_audio_codec_priv_t *c;

	if (name) {
		if (strcmp(name, "mp3") == 0) {
			c = my_audio_codec_create_mpg123();
		}
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
