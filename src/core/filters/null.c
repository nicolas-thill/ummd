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

#include "core/filters_priv.h"

#include "util/list.h"
#include "util/log.h"
#include "util/mem.h"

typedef struct my_filter_data_s my_filter_data_t;

struct my_filter_data_s {
	my_filter_priv_t _inherited;
};

#define MY_FILTER_DATA(p) ((my_filter_data_t *)(p))
#define MY_FILTER_DATA_SIZE (sizeof(my_filter_data_t))


static my_filter_t *my_filter_null_create(my_filter_conf_t *conf)
{
	my_filter_t *filter;

	filter = my_filter_priv_create(conf, MY_FILTER_DATA_SIZE);
	if (!filter) {
		my_log(MY_LOG_ERROR, "core/filter: error allocating data for filter #%d '%s'", conf->index, conf->name);
		goto _MY_ERR_alloc;
	}

	return filter;

	my_filter_priv_destroy(filter);
_MY_ERR_alloc:
	return NULL;
}

static void my_filter_null_destroy(my_filter_t *filter)
{
	my_filter_priv_destroy(filter);
}

my_filter_impl_t my_filter_null = {
	.name = "null",
	.desc = "Null filter",
	.create = my_filter_null_create,
	.destroy = my_filter_null_destroy,
};
