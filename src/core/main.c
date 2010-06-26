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

#include <event.h>

#include "core.h"

#include "core/controls.h"
#include "core/filters.h"
#include "core/sources.h"
#include "core/targets.h"

#include "util/log.h"
#include "util/mem.h"

typedef struct my_core my_core_priv_t;

struct my_core_priv {
	my_core_t base;
	struct event_base *evb;
};

#define MY_CORE(p) ((my_core_t *)p)
#define MY_CORE_PRIV(p) ((my_core_priv_t *)p)

my_core_t *my_core_create(void)
{
	my_core_t *core;

	core = my_mem_alloc(sizeof(my_core_priv_t));
	if (!core) {
		MY_ERROR("core: error creating data structure (%s)" , strerror(errno));
		exit(1);
	}
	
	core->controls = my_list_create();
	if (core->controls == NULL) {
		MY_ERROR("core: error creating control list (%s)" , strerror(errno));
		exit(1);
	}

	core->filters = my_list_create();
	if (core->filters == NULL) {
		MY_ERROR("core: error creating filter list (%s)" , strerror(errno));
		exit(1);
	}

	core->sources = my_list_create();
	if (core->sources == NULL) {
		MY_ERROR("core: error creating source list (%s)" , strerror(errno));
		exit(1);
	}

	core->targets = my_list_create();
	if (core->targets == NULL) {
		MY_ERROR("core: error creating target list (%s)" , strerror(errno));
		exit(1);
	}

	return core;
}

void my_core_destroy(my_core_t *core) {
	my_list_destroy(core->controls);
	my_list_destroy(core->filters);
	my_list_destroy(core->sources);
	my_list_destroy(core->targets);

	my_mem_free(core);
}

int my_core_init(my_core_t *core, my_conf_t *conf)
{
	my_core_priv_t *c = (my_core_priv_t *)core;

	my_control_register_all();
	my_filter_register_all();
	my_source_register_all();
	my_target_register_all();

	return 0;
}

#ifdef MY_DEBUGGING

void my_core_dump(my_core_t *core)
{
	my_control_dump_all();
	my_filter_dump_all();
	my_source_dump_all();
	my_target_dump_all();
}

#endif /* MY_DEBUGGING */
