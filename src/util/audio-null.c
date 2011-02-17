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

#include "util/audio.h"

#include "util/log.h"
#include "util/mem.h"


static my_audio_codec_t *my_audio_codec_null_create(void)
{
	my_audio_codec_t *c;

	c = my_mem_alloc(sizeof(*c));
	if (!c) {
		my_log(MY_LOG_ERROR, "audio/%s: error allocating codec data (%d: %s)", c->impl->name, errno, strerror(errno));
		goto _MY_ERR_mem_alloc;
	}
	
	return c;

	my_mem_free(c);
_MY_ERR_mem_alloc:
	return NULL;
}

static void my_audio_codec_null_destroy(my_audio_codec_t *c)
{
	my_mem_free(c);
}


static int my_audio_codec_null_process(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen)
{
	int n = MIN(*ilen, *olen);

	memcpy(obuf, ibuf, n);
	*ilen = n;
	*olen = n;

	return n;
}


my_audio_codec_impl_t my_audio_codec_null = {
	.name = "null",
	.desc = "Null audio encoder/decoder",
	.init = NULL,
	.fini = NULL,
	.create = my_audio_codec_null_create,
	.destroy = my_audio_codec_null_destroy,
	.decode = my_audio_codec_null_process,
	.encode = my_audio_codec_null_process,
};
