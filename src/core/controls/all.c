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

#include "core/ports.h"

#include "util/list.h"
#include "util/log.h"
#include "util/mem.h"
#include "util/prop.h"
#include "util/url.h"

static my_list_t my_controls;

#define MY_CONTROL_REGISTER(x) { \
	extern my_port_impl_t my_control_##x; \
	my_control_register(&my_control_##x); \
}

static my_port_impl_t *my_control_impl_find(my_port_conf_t *conf)
{
	my_port_impl_t *impl;
	my_node_t *node;
	char *impl_name;
	char *url;
	char url_prot[10];

	impl_name = my_prop_lookup(conf->properties, "type");
	if (!impl_name) {
		url = my_prop_lookup(conf->properties, "url");
		if (url) {
			my_url_split(
				url_prot, sizeof(url_prot),
				NULL, 0, /* auth */
				NULL, 0, /* hostname */
				NULL,    /* port */
				NULL, 0, /* path */
				url
			);
			impl_name = url_prot;
		} else {
			impl_name = "fifo";
		}
	}
	for (node = my_controls.head; node; node = node->next) {
		impl = MY_PORT_IMPL(node->data);
		if (strcmp(impl->name, impl_name) == 0) {
			return impl;
		}
	}

	my_log(MY_LOG_ERROR, "core/control: unknown type '%s' for control #%d '%s'", impl_name, conf->index, conf->name);

	return NULL;
}

my_port_t *my_control_create(my_port_conf_t *conf)
{
	my_port_impl_t *impl;

	impl = my_control_impl_find(conf);
	if (!impl) {
		return NULL;
	}

	return my_port_create(conf, impl);
}

void my_control_destroy(my_port_t *control)
{
	my_port_destroy(control);
}

static int my_control_create_fn(void *data, void *user, int flags)
{
	my_core_t *core = MY_CORE(user);
	my_port_t *control;
	my_port_conf_t *conf = MY_PORT_CONF(data);

	control = my_control_create(conf);
	if (control) {
		my_list_enqueue(core->controls, control);
		control->core = core;
	}

	return 0;
}

int my_control_create_all(my_core_t *core, my_conf_t *conf)
{
	MY_DEBUG("core/control: creating all controls");
	return my_list_iter(conf->controls, my_control_create_fn, core);
}


int my_control_destroy_all(my_core_t *core)
{
	my_port_t *control;

	MY_DEBUG("core/control: destroying all controls");
	while (control = my_list_dequeue(core->controls)) {
		my_control_destroy(control);
	}
}


static int my_control_open_fn(void *data, void *user, int flags)
{
	my_port_t *control = MY_PORT(data);

	MY_PORT_GET_IMPL(control)->open(control);

	return 0;
}

int my_control_open_all(my_core_t *core)
{
	MY_DEBUG("core/control: opening all controls");
	return my_list_iter(core->controls, my_control_open_fn, core);
}


static int my_control_close_fn(void *data, void *user, int flags)
{
	my_port_t *control = MY_PORT(data);

	MY_PORT_GET_IMPL(control)->close(control);

	return 0;
}

int my_control_close_all(my_core_t *core)
{
	MY_DEBUG("core/control: closing all controls");
	return my_list_iter(core->controls, my_control_close_fn, core);
}


void my_control_register(my_port_impl_t *impl)
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
	my_port_impl_t *impl = MY_PORT_IMPL(data);

	MY_DEBUG("\t{");
	MY_DEBUG("\t\tname=\"%s\";", impl->name);
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
