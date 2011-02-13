/*
 *  ummd ( Micro MultiMedia Daemon )
 *
 *  Copyright (C) 2011 Nicolas Thill <nicolas.thill@gmail.com>
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

#ifndef __MY_AUDIO_H
#define __MY_AUDIO_H

typedef void *my_audio_codec_t;

extern int my_audio_codec_init(void);

extern my_audio_codec_t *my_audio_codec_create(char *name);
extern void my_audio_codec_destroy(my_audio_codec_t *c);

extern int my_audio_codec_encode(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen);
extern int my_audio_codec_decode(my_audio_codec_t *c, void *ibuf, int *ilen, void *obuf, int *olen);

#endif /* __MY_AUDIO_H */
