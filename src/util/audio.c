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

#include "util/list.h"
#include "util/log.h"
#include "util/mem.h"


static my_list_t my_audio_codecs;

#define MY_AUDIO_CODEC_REGISTER(x) { \
	extern my_audio_codec_impl_t my_audio_codec_##x; \
	my_audio_codec_impl_register(&my_audio_codec_##x); \
}

static my_audio_codec_impl_t *my_audio_codec_impl_find(char *name)
{
	my_audio_codec_impl_t *impl;
	my_node_t *node;

	if (!name) {
		name = "null";
	}

	for (node = my_audio_codecs.head; node; node = node->next) {
		impl = MY_AUDIO_CODEC_IMPL(node->data);
		if (strcmp(impl->name, name) == 0) {
			return impl;
		}
	}

	return NULL;
}

static void my_audio_codec_impl_register(my_audio_codec_impl_t *impl)
{
	my_list_enqueue(&my_audio_codecs, impl);
}


static int my_audio_codec_init_fn(void *data, void *user, int flags)
{
	my_audio_codec_impl_t *impl = MY_AUDIO_CODEC_IMPL(data);
	int rc;

	if (impl->init ) {
		rc = impl->init();
	} else {
		rc = 0;
	}

	return rc;
}

int my_audio_codec_init(void)
{
	MY_AUDIO_CODEC_REGISTER(null);
	MY_AUDIO_CODEC_REGISTER(mp3);
	
	return my_list_iter(&my_audio_codecs, my_audio_codec_init_fn, NULL);
}


static int my_audio_codec_fini_fn(void *data, void *user, int flags)
{
	my_audio_codec_impl_t *impl = MY_AUDIO_CODEC_IMPL(data);
	int rc;

	if (impl->fini ) {
		rc = impl->fini();
	} else {
		rc = 0;
	}

	return rc;
}

int my_audio_codec_fini(void)
{
	return my_list_iter(&my_audio_codecs, my_audio_codec_init_fn, NULL);
}


my_audio_codec_t *my_audio_codec_create(char *name)
{
	my_audio_codec_impl_t *impl;
	my_audio_codec_t *c;

	impl = my_audio_codec_impl_find(name);
	if (!impl) {
		goto _MY_ERR_find;
	}

	c = impl->create();
	if (!c) {
		goto _MY_ERR_create;
	}

	c->impl = impl;

	return c;

_MY_ERR_create:
_MY_ERR_find:
	return NULL;
}

void my_audio_codec_destroy(my_audio_codec_t *c)
{
	c->impl->destroy(c);
}


int my_audio_encode(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen)
{
	return c->impl->encode(c, ibuf, ilen, obuf, olen);
}

int my_audio_decode(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen)
{
	return c->impl->decode(c, ibuf, ilen, obuf, olen);
}


int my_audio_decode_not_implemented(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen)
{
	my_log(MY_LOG_ERROR, "audio/%s: decoding not implemented", c->impl->name);
	return -1;
}

int my_audio_encode_not_implemented(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen)
{
	my_log(MY_LOG_ERROR, "audio/%s: encoding not implemented", c->impl->name);
	return -1;
}

