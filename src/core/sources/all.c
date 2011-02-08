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

static my_list_t my_sources;

#define MY_SOURCE_REGISTER(x) { \
	extern my_port_impl_t my_source_##x; \
	my_port_impl_register(&my_sources, &my_source_##x); \
}

static my_port_impl_t *my_source_impl_find(my_port_conf_t *conf)
{
	my_port_impl_t *impl;
	my_node_t *node;
	char *impl_name;
	char *conf_type;
	char *url;
	char url_prot[10];
	char url_host[255];
	char buf[255];

	impl_name = "file";

	url = my_prop_lookup(conf->properties, "url");
	if (url) {
		my_url_split(
			url_prot, sizeof(url_prot),
			NULL, 0, /* auth */
			url_host, sizeof(url_host),
			NULL,    /* port */
			NULL, 0, /* path */
			url
		);
		if (strlen(url_host) > 0) {
			if (strlen(url_prot) == 0) {
				strncpy(url_prot, "udp", sizeof(url_prot));
			}
			conf_type = my_prop_lookup(conf->properties, "type");
			if (!conf_type) {
				conf_type = "client";
			}
			snprintf(buf, sizeof(buf), "%s-%s", url_prot, conf_type);
			impl_name = buf;
		} else {
			if (strlen(url_prot) > 0) {
				impl_name = url_prot;
			}
		}
	}

	impl = my_port_impl_lookup(&my_sources, impl_name);
	if (!impl) {
		my_log(MY_LOG_ERROR, "core/source: unknown type '%s' for source #%d '%s'", impl_name, conf->index, conf->name);
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

	port = my_port_create(conf, impl);
	if (port) {
		my_list_enqueue(core->sources, port);
		port->core = core;
	}

	return 0;
}

int my_source_create_all(my_core_t *core, my_conf_t *conf)
{
	MY_DEBUG("core/source: creating all sources");
	return my_list_iter(conf->sources, my_source_create_fn, core);
}

int my_source_destroy_all(my_core_t *core)
{
	return my_port_destroy_all(core->sources);
}

int my_source_open_all(my_core_t *core)
{
	MY_DEBUG("core/source: opening all sources");
	return my_port_open_all(core->sources);
}

int my_source_close_all(my_core_t *core)
{
	MY_DEBUG("core/source: closing all sources");
	return my_port_close_all(core->sources);
}


void my_source_register_all(void)
{
	MY_SOURCE_REGISTER(file);
/*
	MY_SOURCE_REGISTER(sock);
	MY_SOURCE_REGISTER(http_client);
	MY_SOURCE_REGISTER(rtp_client);
	MY_SOURCE_REGISTER(udp_client);
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
