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

#include <stdlib.h>
#include <string.h>

#include "util/rbuf.h"

#include "util/mem.h"

my_rbuf_t *my_rbuf_create(int size)
{
	my_rbuf_t *rbuf;
	
	rbuf = my_mem_alloc(sizeof(my_rbuf_t));
	if (!rbuf) {
		goto _MY_ERR_alloc;
	}

	rbuf->data = my_mem_alloc(size + 1);
	if( !(rbuf->data)) {
		goto _MY_ERR_alloc_data;
	}
	
	rbuf->size = size;
	rbuf->off_get = 0;
	rbuf->off_put = 0;
	
	return rbuf;

	my_mem_free(rbuf->data);
_MY_ERR_alloc_data:
	my_mem_free(rbuf);
_MY_ERR_alloc:
	return NULL;
}

void my_rbuf_destroy(my_rbuf_t *rbuf)
{
	my_mem_free(rbuf->data);
	my_mem_free(rbuf);
}

int my_rbuf_get_avail(my_rbuf_t *rbuf)
{
	int avail;

	if (rbuf->off_get < rbuf->off_put) {
		avail = rbuf->off_put - rbuf->off_get;
	} else {
		avail = (rbuf->off_put - rbuf->off_get + rbuf->size) % rbuf->size;
	}

	return avail;
}

int my_rbuf_put_avail(my_rbuf_t *rbuf)
{
	int avail;

	if (rbuf->off_get > rbuf->off_put) {
		avail = rbuf->off_get - rbuf->off_put;
	} else if (rbuf->off_get < rbuf->off_put) {
		avail = (rbuf->off_get - rbuf->off_put + rbuf->size) % rbuf->size;
	} else {
		avail = rbuf->size;
	}

	return avail - 1;
}


int my_rbuf_get(my_rbuf_t *rbuf, char *data, int size)
{
	int avail, off;
	
	avail = my_rbuf_get_avail(rbuf);
	if (!avail) {
		return 0;
	}

	if (size > avail) {
		size = avail;
	}
	avail = size;
	off = rbuf->off_get + size;
	if (off > rbuf->size) {
		off %= rbuf->size;
		memcpy(data + rbuf->size - rbuf->off_get, rbuf->data, off);
		avail -= off;
	}
	memcpy(data, rbuf->data + rbuf->off_get, avail);
	rbuf->off_get = off;

	return size;
}

int my_rbuf_put(my_rbuf_t *rbuf, char *data, int size)
{
	int avail, off;
	
	avail = my_rbuf_put_avail(rbuf);
	if (!avail) {
		return 0;
	}

	if (size > avail) {
		size = avail;
	}
	avail = size;
	off = rbuf->off_put + size;
	if (off > rbuf->size) {
		off %= rbuf->size;
		memcpy(rbuf->data, data + rbuf->size - rbuf->off_put, off);
		avail -= off;
	}
	memcpy(rbuf->data + rbuf->off_put, data, avail);
	rbuf->off_put = off;

	return size;
}
