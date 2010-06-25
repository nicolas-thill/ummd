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

#include "core.h"

#include "core/controls.h"
#include "core/filters.h"
#include "core/sources.h"
#include "core/targets.h"

#include "util/log.h"
#include "util/mem.h"

void my_core_init(my_core_t *core, my_conf_t *conf)
{
	my_mem_zero(core, sizeof(*core));
	
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

	my_core_control_register_all(core);
	my_core_filter_register_all(core);
	my_core_source_register_all(core);
	my_core_target_register_all(core);
}


#ifdef MY_DEBUGGING

void my_core_dump(my_core_t *core)
{
	my_core_control_dump_all(core);
}

#endif /* MY_DEBUGGING */
