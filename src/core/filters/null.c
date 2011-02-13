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

#include "core/ports.h"

#include "util/list.h"
#include "util/log.h"
#include "util/mem.h"

typedef struct my_filter_priv_s my_filter_priv_t;

struct my_filter_priv_s {
	my_dport_t _inherited;
};

#define MY_FILTER(p) ((my_filter_priv_t *)(p))
#define MY_FILTER_SIZE (sizeof(my_filter_priv_t))


static my_port_t *my_filter_null_create(my_core_t *core, my_port_conf_t *conf)
{
	my_port_t *port;

	port = my_port_create_priv(MY_FILTER_SIZE);
	if (!port) {
		my_log(MY_LOG_ERROR, "core/filter: error allocating data for filter #%d '%s'", conf->index, conf->name);
		goto _MY_ERR_alloc;
	}

	return port;

	my_port_destroy_priv(port);
_MY_ERR_alloc:
	return NULL;
}

static void my_filter_null_destroy(my_port_t *port)
{
	my_port_destroy_priv(port);
}

my_port_impl_t my_filter_null = {
	.name = "null",
	.desc = "Null filter",
	.create = my_filter_null_create,
	.destroy = my_filter_null_destroy,
};
