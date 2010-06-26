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

#include "core/controls.h"

#include "util/list.h"
#include "util/log.h"

#define MY_CONTROL_REGISTER(c,x) { \
	extern my_control_impl_t my_control_##x; \
	my_control_register((c), &my_control_##x); \
}

static my_control_impl_t *my_control_find_impl(my_core_t *core, char *name)
{
	my_control_impl_t *impl;
	my_node_t *node;

	for (node = core->controls->head; node; node = node->next) {
		impl = (my_control_impl_t *)(node->data);
		if (strcmp(impl->name, name) == 0) {
			return impl;
		}
	}
	return NULL;
}

my_control_t *my_control_create(my_core_t *core, my_control_conf_t *conf)
{
	my_control_t *control;
	my_control_impl_t *impl;
	
	impl = my_control_find_impl(core, conf->type);
	if (!impl) {
		MY_ERROR("unknown control interface (%s)" , conf->type);
		return NULL;
	}
	control = impl->create(conf);
	if (!control) {
		MY_ERROR("error creating control (%s)" , conf->name);
		return NULL;
	}

	control->core = core;
	control->conf = conf;
	control->impl = impl;

	return control;
}

void my_control_register(my_core_t *core, my_control_impl_t *impl)
{
	my_list_queue(core->controls, impl);
}

void my_control_register_all(my_core_t *core)
{
	MY_CONTROL_REGISTER(core, fifo);
	MY_CONTROL_REGISTER(core, osc);
/*
	MY_CONTROL_REGISTER(core, http);
	MY_CONTROL_REGISTER(core, sock);
*/
}

#ifdef MY_DEBUGGING

static int my_control_dump(void *data, void *user, int flags)
{
	my_control_impl_t *impl = (my_control_impl_t *)data;

	MY_DEBUG("\t{");
	MY_DEBUG("\t\tname=\"%s\";", impl->name);
	MY_DEBUG("\t\tdescription=\"%s\";", impl->desc);
	MY_DEBUG("\t}%s", flags & MY_LIST_ITER_FLAG_LAST ? "" : ",");

	return 0;
}


void my_control_dump_all(my_core_t *core)
{
	MY_DEBUG("# registered control interfaces");
	MY_DEBUG("controls = (");
	my_list_iter(core->controls, my_control_dump, NULL);
	MY_DEBUG(");");

}

#endif /* MY_DEBUGGING */
