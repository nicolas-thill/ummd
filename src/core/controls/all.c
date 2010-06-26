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

static my_list_t my_controls;

#define MY_CONTROL_REGISTER(x) { \
	extern my_control_impl_t my_control_##x; \
	my_control_register(&my_control_##x); \
}

static my_control_impl_t *my_control_find_impl(char *name)
{
	my_control_impl_t *impl;
	my_node_t *node;

	for (node = my_controls.head; node; node = node->next) {
		impl = (my_control_impl_t *)(node->data);
		if (strcmp(impl->name, name) == 0) {
			return impl;
		}
	}

	return NULL;
}

static my_control_t *my_control_create(my_core_t *core, my_control_conf_t *conf)
{
	my_control_t *control;
	my_control_impl_t *impl;
	
	impl = my_control_find_impl(conf->type);
	if (!impl) {
		my_log(MY_LOG_ERROR, "core/control: unknown control interface '%s' for control '%s'", conf->type, conf->name);
		return NULL;
	}
	control = impl->create(conf);
	if (!control) {
		my_log(MY_LOG_ERROR, "core/control: error creating control '%s'", conf->name);
		return NULL;
	}

	control->core = core;
	control->conf = conf;
	control->impl = impl;

	return control;
}

static void my_control_destroy(my_control_t *control)
{
	control->impl->destroy(control);
}

int my_control_create_one(void *data, void *user, int flags)
{
	my_control_conf_t *conf = (my_control_conf_t *)data;
	my_core_t *core = (my_core_t *)user;
	my_control_t *control;

	control = my_control_create(core, conf);
	if (control) {
		my_list_enqueue(core->controls, control);
	}

	return 0;
}

int my_control_destroy_one(void *data, void *user, int flags)
{
	my_control_t *control = (my_control_conf_t *)data;

	my_control_destroy(control);
}

int my_control_create_all(my_core_t *core, my_conf_t *conf)
{
	return my_list_iter(conf->controls, my_control_create_one, core);
}

int my_control_destroy_all(my_core_t *core)
{
	return my_list_iter(core->controls, my_control_destroy_one, core);
}

static void my_control_register(my_control_impl_t *impl)
{
	my_list_enqueue(&my_controls, impl);
}

void my_control_register_all(void)
{
	MY_CONTROL_REGISTER(fifo);
/*
	MY_CONTROL_REGISTER(osc);
	MY_CONTROL_REGISTER(http);
	MY_CONTROL_REGISTER(sock);
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


void my_control_dump_all(void)
{
	MY_DEBUG("# registered control interfaces");
	MY_DEBUG("controls = (");
	my_list_iter(&my_controls, my_control_dump, NULL);
	MY_DEBUG(");");

}

#endif /* MY_DEBUGGING */
