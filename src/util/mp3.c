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

#include "core/ports.h"

#include "util/log.h"
#include "util/mem.h"
#include "util/rbuf.h"

typedef struct my_mp3_dec_data_s my_mp3_dec_data_t;

#define MY_MP3_BUF_SIZE  AVCODEC_MAX_AUDIO_FRAME_SIZE

struct my_mp3_dec_data_s {
	struct AVCodecContext *context;
	struct AVPacket packet;
};

int my_mp3_init(void)
{
	avcodec_init();
	avcodec_register_all();

	return 0;
}

my_mp3_dec_data_t *my_mp3_dec_create(void)
{
	my_mp3_dec_data_t *dec;
	AVCodec *codec;

	dec = my_mem_alloc(sizeof(*dec));
	if (!dec) {
		my_log(MY_LOG_ERROR, "mp3: error allocating codec data (%d: %s)", errno, strerror(errno));
		goto _ERR_mem_alloc;
	}
	
	codec = avcodec_find_decoder(CODEC_ID_MP3);
	if (!codec) {
		my_log(MY_LOG_ERROR, "mp3: error finding decoder");
		goto _ERR_avcodec_find_decoder;
	}

	dec->context = avcodec_alloc_context();
	if (!dec->context) {
		my_log(MY_LOG_ERROR, "mp3: error allocating context");
		goto _ERR_avcodec_alloc_context;
	}

	if (avcodec_open(dec->context, codec) < 0) {
		my_log(MY_LOG_ERROR, "mp3: error opening decoder");
		goto _ERR_avcodec_open;
	}

	av_init_packet(&(dec->packet));

	return (my_mp3_dec_data_t *)dec;

	avcodec_close(dec->context);
_ERR_avcodec_open:
	av_free(dec->context);
_ERR_avcodec_alloc_context:
_ERR_avcodec_find_decoder:
	my_mem_free(dec);
_ERR_mem_alloc:
	return NULL;
}

int my_mp3_dec_destroy(my_mp3_dec_data_t *dec)
{
	avcodec_close(dec->context);
	av_free(dec->context);
	my_mem_free(dec);
}

int my_mp3_dec_process(my_mp3_dec_data_t *dec, my_port_t *source, my_port_t *target)
{
	uint8_t ibuf[MY_MP3_BUF_SIZE], obuf[MY_MP3_BUF_SIZE];
	int ilen, olen, n;

	dec->packet.data = ibuf;
	dec->packet.size = ilen;
	
	olen = MY_MP3_BUF_SIZE;
	n = avcodec_decode_audio3(dec->context, (short *)obuf, &olen, &(dec->packet));
	if (n < 0) {
		my_log(MY_LOG_ERROR, "mp3: error decoding audio");
	}

	return 0;
}
