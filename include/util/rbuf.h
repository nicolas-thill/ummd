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

#ifndef __MY_UTIL_RBUF_H
#define __MY_UTIL_RBUF_H

typedef struct my_rbuf_s my_rbuf_t;

struct my_rbuf_s {
	char *data;
	int size;
	int off_get;
	int off_put;
};

extern my_rbuf_t *my_rbuf_create(int size);
extern void my_rbuf_destroy(my_rbuf_t *rbuf);

extern int my_rbuf_get_avail(my_rbuf_t *rbuf);
extern int my_rbuf_put_avail(my_rbuf_t *rbuf);

extern int my_rbuf_get(my_rbuf_t *rbuf, char *data, int size);
extern int my_rbuf_put(my_rbuf_t *rbuf, char *data, int size);

#endif /* __MY_UTIL_RBUF_H */
