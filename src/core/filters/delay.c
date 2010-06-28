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

#include "core/filters.h"

#include "util/list.h"
#include "util/log.h"
#include "util/mem.h"

typedef struct my_filter_priv my_filter_priv_t;

struct my_filter_priv {
	my_filter_t base;
};

#define MY_FILTER_PRIV(p) ((my_filter_priv_t *)(p))

static my_filter_t *my_filter_delay_create(my_filter_conf_t *conf)
{
	my_filter_t *filter;

	filter = my_mem_alloc(sizeof(my_filter_priv_t));
	if (!filter) {
		my_log(MY_LOG_ERROR, "core/filter: error allocating data for filter #%d '%s'", conf->index, conf->name);
		goto _MY_ERR_alloc;
	}

	return filter;

	my_mem_free(filter);
_MY_ERR_alloc:
	return NULL;
}

static void my_filter_delay_destroy(my_filter_t *filter)
{
	my_mem_free(filter);
}

my_filter_impl_t my_filter_delay = {
	.id = 1,
	.name = "delay",
	.desc = "Delay filter",
	.create = my_filter_delay_create,
	.destroy = my_filter_delay_destroy,
};
