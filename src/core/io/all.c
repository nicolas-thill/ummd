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

static my_list_t my_controls;
static my_list_t my_sources;
static my_list_t my_targets;

#define MY_CONTROL_REGISTER(x) { \
	extern my_port_impl_t my_control_##x; \
	my_port_impl_register(&my_controls, &my_control_##x); \
}

	#define MY_SOURCE_REGISTER(x) { \
	extern my_port_impl_t my_source_##x; \
	my_port_impl_register(&my_sources, &my_source_##x); \
}

#define MY_TARGET_REGISTER(x) { \
	extern my_port_impl_t my_target_##x; \
	my_port_impl_register(&my_targets, &my_target_##x); \
}


static my_port_impl_t *my_control_impl_find(my_port_conf_t *conf)
{
	my_port_impl_t *impl;
	char *impl_name;

	impl_name = my_prop_lookup(conf->properties, "type");
	if (!impl_name) {
		impl_name = "fifo";
	}

	impl = my_port_impl_lookup(&my_controls, impl_name);
	if (!impl) {
		my_log(MY_LOG_ERROR, "core/%s: unknown type '%s'", conf->name, impl_name);
	}

	return impl;
}

static int my_control_create_fn(void *data, void *user, int flags)
{
	my_core_t *core = MY_CORE(user);
	my_port_t *port;
	my_port_conf_t *conf = MY_PORT_CONF(data);
	my_port_impl_t *impl;

	impl = my_control_impl_find(conf);
	if (!impl) {
		return 0;
	}

	port = my_port_create(core, conf, impl);
	if (port) {
		my_list_enqueue(core->controls, port);
	}

	return 0;
}

int my_control_create_all(my_core_t *core, my_conf_t *conf)
{
	MY_DEBUG("core/controls: creating all");
	return my_list_iter(conf->controls, my_control_create_fn, core);
}

int my_control_destroy_all(my_core_t *core)
{
	MY_DEBUG("core/controls: destroying all");
	return my_port_destroy_all(core->controls);
}

int my_control_open_all(my_core_t *core)
{
	MY_DEBUG("core/controls: opening all");
	return my_port_open_all(core->controls);
}

int my_control_close_all(my_core_t *core)
{
	MY_DEBUG("core/controls: closing all");
	return my_port_close_all(core->controls);
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


static my_port_impl_t *my_source_impl_find(my_port_conf_t *conf)
{
	my_port_impl_t *impl;
	my_node_t *node;
	char *impl_name;

	impl_name = my_prop_lookup(conf->properties, "type");
	if (!impl_name) {
		impl_name = "file";
	}

	impl = my_port_impl_lookup(&my_sources, impl_name);
	if (!impl) {
		my_log(MY_LOG_ERROR, "core/%s: unknown type '%s'", conf->name, impl_name);
	}

	return impl;
}

static int my_source_create_fn(void *data, void *user, int flags)
{
	my_core_t *core = MY_CORE(user);
	my_port_t *port;
	my_port_conf_t *conf = MY_PORT_CONF(data);
	my_port_impl_t *impl;

	impl = my_source_impl_find(conf);
	if (!impl) {
		return 0;
	}

	port = my_port_create(core, conf, impl);
	if (port) {
		my_list_enqueue(core->sources, port);
	}

	return 0;
}

int my_source_create_all(my_core_t *core, my_conf_t *conf)
{
	MY_DEBUG("core/sources: creating all");
	return my_list_iter(conf->sources, my_source_create_fn, core);
}

int my_source_destroy_all(my_core_t *core)
{
	MY_DEBUG("core/sources: destroying all");
	return my_port_destroy_all(core->sources);
}

int my_source_open_all(my_core_t *core)
{
	MY_DEBUG("core/sources: opening all");
	return my_port_open_all(core->sources);
}

int my_source_close_all(my_core_t *core)
{
	MY_DEBUG("core/sources: closing all");
	return my_port_close_all(core->sources);
}


void my_source_register_all(void)
{
	MY_SOURCE_REGISTER(file);
	MY_SOURCE_REGISTER(udp);
/*
	MY_SOURCE_REGISTER(sock);
	MY_SOURCE_REGISTER(http_client);
	MY_SOURCE_REGISTER(rtp_client);
*/
}

#ifdef MY_DEBUGGING

static int my_source_dump_fn(void *data, void *user, int flags)
{
	my_port_impl_t *impl = MY_PORT_IMPL(data);

	MY_DEBUG("\t{");
	MY_DEBUG("\t\tname=\"%s\";", impl->name);
	MY_DEBUG("\t\tdescription=\"%s\";", impl->desc);
	MY_DEBUG("\t}%s", flags & MY_LIST_ITER_FLAG_LAST ? "" : ",");

	return 0;
}

void my_source_dump_all(void)
{
	MY_DEBUG("# registered sources");
	MY_DEBUG("sources = (");
	my_list_iter(&my_sources, my_source_dump_fn, NULL);
	MY_DEBUG(");");

}

#endif /* MY_DEBUGGING */


static my_port_impl_t *my_target_impl_find(my_port_conf_t *conf)
{
	my_port_impl_t *impl;
	my_node_t *node;
	char *impl_name;

	impl_name = my_prop_lookup(conf->properties, "type");
	if (!impl_name) {
		impl_name = "file";
	}

	impl = my_port_impl_lookup(&my_targets, impl_name);
	if (!impl) {
		my_log(MY_LOG_ERROR, "core/%s: unknown type '%s'", conf->name, impl_name);
	}

	return impl;
}

static int my_target_create_fn(void *data, void *user, int flags)
{
	my_core_t *core = MY_CORE(user);
	my_port_t *port;
	my_port_conf_t *conf = MY_PORT_CONF(data);
	my_port_impl_t *impl;

	impl = my_target_impl_find(conf);
	if (!impl) {
		return 0;
	}

	port = my_port_create(core, conf, impl);
	if (port) {
		my_list_enqueue(core->targets, port);
	}

	return 0;
}

int my_target_create_all(my_core_t *core, my_conf_t *conf)
{
	MY_DEBUG("core/targets: creating all");
	return my_list_iter(conf->targets, my_target_create_fn, core);
}

int my_target_destroy_all(my_core_t *core)
{
	MY_DEBUG("core/targets: destroying all");
	return my_port_destroy_all(core->targets);
}

int my_target_open_all(my_core_t *core)
{
	MY_DEBUG("core/targets: opening all");
	return my_port_open_all(core->targets);
}

int my_target_close_all(my_core_t *core)
{
	MY_DEBUG("core/targets: closing all");
	return my_port_close_all(core->targets);
}

void my_target_register_all(void)
{
	MY_TARGET_REGISTER(file);
#ifdef HAVE_OSS
	MY_TARGET_REGISTER(oss);
#endif
/*
	MY_TARGET_REGISTER(sock);
	MY_TARGET_REGISTER(http_client);
	MY_TARGET_REGISTER(rtp_client);
	MY_TARGET_REGISTER(udp_client);
*/
	MY_TARGET_REGISTER(udp);
}

#ifdef MY_DEBUGGING

static int my_target_dump_fn(void *data, void *user, int flags)
{
	my_port_impl_t *impl = MY_PORT_IMPL(data);

	MY_DEBUG("\t{");
	MY_DEBUG("\t\tname=\"%s\";", impl->name);
	MY_DEBUG("\t\tdescription=\"%s\";", impl->desc);
	MY_DEBUG("\t}%s", flags & MY_LIST_ITER_FLAG_LAST ? "" : ",");

	return 0;
}

void my_target_dump_all(void)
{
	MY_DEBUG("# registered targets");
	MY_DEBUG("targets = (");
	my_list_iter(&my_targets, my_target_dump_fn, NULL);
	MY_DEBUG(");");

}

#endif /* MY_DEBUGGING */
