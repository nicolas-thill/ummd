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

static my_control_impl_t *my_control_find_impl(my_control_conf_t *conf)
{
	my_control_impl_t *impl;
	my_node_t *node;
	char *impl_name;
	char url_prot[10];

	impl_name = conf->type;
	if (!impl_name) {
		url_split(
			url_prot, sizeof(url_prot),
			NULL, 0, /* auth */
			NULL, 0, /* hostname */
			NULL,    /* port */
			NULL, 0, /* path */
			conf->url
		);
		impl_name = url_prot;
	}
	for (node = my_controls.head; node; node = node->next) {
		impl = MY_CONTROL_IMPL(node->data);
		if (strcmp(impl->name, impl_name) == 0) {
			return impl;
		}
	}

	my_log(MY_LOG_ERROR, "core/control: unknown type '%s' for control #%d '%s'", impl_name, conf->index, conf->name);

	return NULL;
}

static my_control_t *my_control_create(my_core_t *core, my_control_conf_t *conf)
{
	my_control_t *control;
	my_control_impl_t *impl;
	
	impl = my_control_find_impl(conf);
	if (!impl) {
		return NULL;
	}
	control = impl->create(conf);
	if (!control) {
		my_log(MY_LOG_ERROR, "core/control: error creating control #%d '%s'", conf->index, conf->name);
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

static int my_control_create_fn(void *data, void *user, int flags)
{
	my_core_t *core = MY_CORE(user);
	my_control_t *control;
	my_control_conf_t *conf = MY_CONTROL_CONF(data);

	control = my_control_create(core, conf);
	if (control) {
		my_list_enqueue(core->controls, control);
	}

	return 0;
}

static int my_control_open_fn(void *data, void *user, int flags)
{
	my_control_t *control = MY_CONTROL(data);

	control->impl->open(control);

	return 0;
}

static int my_control_close_fn(void *data, void *user, int flags)
{
	my_control_t *control = MY_CONTROL(data);

	control->impl->close(control);

	return 0;
}

int my_control_create_all(my_core_t *core, my_conf_t *conf)
{
	MY_DEBUG("core/control: creating all controls");
	return my_list_iter(conf->controls, my_control_create_fn, core);
}

int my_control_destroy_all(my_core_t *core)
{
	my_control_t *control;

	MY_DEBUG("core/control: destroying all controls");
	while (control = my_list_dequeue(core->controls)) {
		my_control_destroy(control);
	}
}

int my_control_open_all(my_core_t *core)
{
	MY_DEBUG("core/control: opening all controls");
	return my_list_iter(core->controls, my_control_open_fn, core);
}

int my_control_close_all(my_core_t *core)
{
	MY_DEBUG("core/control: closing all controls");
	return my_list_iter(core->controls, my_control_close_fn, core);
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
*/
	MY_CONTROL_REGISTER(sock);
}

#ifdef MY_DEBUGGING

static int my_control_dump_fn(void *data, void *user, int flags)
{
	my_control_impl_t *impl = MY_CONTROL_IMPL(data);

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
	my_list_iter(&my_controls, my_control_dump_fn, NULL);
	MY_DEBUG(");");

}

#endif /* MY_DEBUGGING */
