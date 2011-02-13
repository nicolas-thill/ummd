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
#include "util/prop.h"

static my_list_t my_filters;

#define MY_FILTER_REGISTER(x) { \
	extern my_port_impl_t my_filter_##x; \
	my_port_impl_register(&my_filters, &my_filter_##x); \
}

static my_port_impl_t *my_filter_impl_find(my_port_conf_t *conf)
{
	my_port_impl_t *impl;
	char *impl_name;

	impl_name = my_prop_lookup(conf->properties, "type");
	if (!impl_name) {
		impl_name = "null";
	}

	impl = my_port_impl_lookup(&my_filters, impl_name);
	if (!impl) {
		my_log(MY_LOG_ERROR, "core/filter: unknown type '%s' for filter #%d '%s'", impl_name, conf->index, conf->name);
	}

	return impl;
}


static int my_filter_create_fn(void *data, void *user, int flags)
{
	my_core_t *core = MY_CORE(user);
	my_port_t *port;
	my_port_conf_t *conf = MY_PORT_CONF(data);
	my_port_impl_t *impl;

	impl = my_filter_impl_find(conf);
	if (!impl) {
		return 0;
	}

	port = my_port_create(core, conf, impl);
	if (port) {
		my_list_enqueue(core->filters, port);
	}

	return 0;
}

int my_filter_create_all(my_core_t *core, my_conf_t *conf)
{
	MY_DEBUG("core/filter: creating all filters");
	return my_list_iter(conf->filters, my_filter_create_fn, core);
}

int my_filter_destroy_all(my_core_t *core)
{
	return my_port_destroy_all(core->filters);
}

void my_filter_register_all(void)
{
	MY_FILTER_REGISTER(null);
	MY_FILTER_REGISTER(delay);
}

#ifdef MY_DEBUGGING

static int my_filter_dump_fn(void *data, void *user, int flags)
{
	my_port_impl_t *impl = MY_PORT_IMPL(data);

	MY_DEBUG("\t{");
	MY_DEBUG("\t\tname=\"%s\";", impl->name);
	MY_DEBUG("\t\tdescription=\"%s\";", impl->desc);
	MY_DEBUG("\t}%s", flags & MY_LIST_ITER_FLAG_LAST ? "" : ",");

	return 0;
}

void my_filter_dump_all(void)
{
	MY_DEBUG("# registered filters");
	MY_DEBUG("filters = (");
	my_list_iter(&my_filters, my_filter_dump_fn, NULL);
	MY_DEBUG(");");

}

#endif /* MY_DEBUGGING */
