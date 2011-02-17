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

#include "util/audio.h"

#include "util/log.h"
#include "util/mem.h"

#include <mpg123.h>


typedef struct my_audio_codec_mp3_s my_audio_codec_mp3_t;

struct my_audio_codec_mp3_s {
	my_audio_codec_t _inherited;
	mpg123_handle *mpg123_h;
};

#define MY_AUDIO_CODEC_MP3(p) ((my_audio_codec_mp3_t *)(p))


static int my_audio_codec_mp3_init(void)
{
	mpg123_init();

	return 0;
}

static int my_audio_codec_mp3_fini(void)
{
	mpg123_exit();

	return 0;
}


static my_audio_codec_t *my_audio_codec_mp3_create(void)
{
	my_audio_codec_t *c;
	int rc;

	c = my_mem_alloc(sizeof(*c));
	if (!c) {
		my_log(MY_LOG_ERROR, "audio/%s: error allocating MP3 codec data (%d: %s)", c->impl->name, errno, strerror(errno));
		goto _MY_ERR_mem_alloc;
	}
	
	MY_AUDIO_CODEC_MP3(c)->mpg123_h = mpg123_new(NULL, &rc);
	if (MY_AUDIO_CODEC_MP3(c)->mpg123_h == NULL) {
		my_log(MY_LOG_ERROR, "audio/%s: error creating codec data (%d: %s)", c->impl->name, rc, mpg123_plain_strerror(rc));
		goto _MY_ERR_mpg123_new;
	}

	rc = mpg123_param(MY_AUDIO_CODEC_MP3(c)->mpg123_h, MPG123_FLAGS, MPG123_QUIET, 0.0);
	if (rc != MPG123_OK) {
		my_log(MY_LOG_ERROR, "audio/%s: error muting codec (%d: %s)", c->impl->name, rc, mpg123_plain_strerror(rc));
		goto _MY_ERR_mpg123_param;
	}

	rc = mpg123_format_none(MY_AUDIO_CODEC_MP3(c)->mpg123_h);
	if (rc != MPG123_OK) {
		my_log(MY_LOG_ERROR, "audio/%s: error setting codec output format (%d: %s)", c->impl->name, rc, mpg123_plain_strerror(rc));
		goto _MY_ERR_mpg123_format;
	}

	rc = mpg123_format(MY_AUDIO_CODEC_MP3(c)->mpg123_h, 44100, MPG123_STEREO, MPG123_ENC_SIGNED_16);
	if (rc != MPG123_OK) {
		my_log(MY_LOG_ERROR, "audio/%s: error setting codec output format (%d: %s)", c->impl->name, rc, mpg123_plain_strerror(rc));
		goto _MY_ERR_mpg123_format;
	}

	rc = mpg123_open_feed(MY_AUDIO_CODEC_MP3(c)->mpg123_h);
	if (rc != MPG123_OK) {
		my_log(MY_LOG_ERROR, "audio/%s: error opening codec feed (%d: %s)", c->impl->name, mpg123_plain_strerror(rc));
		goto _MY_ERR_mpg123_open_feed;
	}

	return c;

_MY_ERR_mpg123_open_feed:
_MY_ERR_mpg123_format:
	mpg123_delete(MY_AUDIO_CODEC_MP3(c)->mpg123_h);
_MY_ERR_mpg123_param:
_MY_ERR_mpg123_new:
	my_mem_free(c);
_MY_ERR_mem_alloc:
	return NULL;
}

static int my_audio_codec_mp3_destroy(my_audio_codec_t *c)
{
	mpg123_delete(MY_AUDIO_CODEC_MP3(c)->mpg123_h);
	my_mem_free(c);

	return 0;
}


static int my_audio_codec_mp3_decode(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen)
{
	size_t n;
	int rc;
	long rate;
	int channels, enc;

	rc = mpg123_feed(MY_AUDIO_CODEC_MP3(c)->mpg123_h, ibuf, *ilen);
	if (rc != MPG123_OK) {
		my_log(MY_LOG_ERROR, "audio/%s: error feeding codec (%d: %s)", c->impl->name, rc, mpg123_plain_strerror(rc));
		goto _MY_ERR_mpg123_feed;
	}

	rc = mpg123_read(MY_AUDIO_CODEC_MP3(c)->mpg123_h, obuf, *olen, &n);
	if (rc == MPG123_NEW_FORMAT) {
		mpg123_getformat(MY_AUDIO_CODEC_MP3(c)->mpg123_h, &rate, &channels, &enc);
		MY_DEBUG("audio/%s: found new stream (rate: %li Hz, channels: %i, encoding: 0x%08x)", c->impl->name, rate, channels, enc);
	} else if ((rc != MPG123_OK) && (rc != MPG123_NEED_MORE)) {
		my_log(MY_LOG_ERROR, "audio/%s: error decoding frame (%d: %s)", c->impl->name, rc, mpg123_plain_strerror(rc));
		goto _MY_ERR_mpg123_read;
	}

	*olen = n;

	return n;

_MY_ERR_mpg123_read:
_MY_ERR_mpg123_feed:
	return -1;
}


my_audio_codec_impl_t my_audio_codec_mp3 = {
	.name = "mp3",
	.desc = "MP3 audio decoder",
	.init = my_audio_codec_mp3_init,
	.fini = my_audio_codec_mp3_fini,
	.create = my_audio_codec_mp3_create,
	.destroy = my_audio_codec_mp3_destroy,
	.decode = my_audio_codec_mp3_decode,
	.encode = my_audio_encode_not_implemented,
};
