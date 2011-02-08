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
	my_source_register(&my_source_##x); \
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

	for (node = my_sources.head; node; node = node->next) {
		impl = MY_PORT_IMPL(node->data);
		if (strcmp(impl->name, impl_name) == 0) {
			return impl;
		}
	}

	my_log(MY_LOG_ERROR, "core/source: unknown type '%s' for source #%d '%s'", impl_name, conf->index, conf->name);

	return NULL;
}

my_port_t *my_source_priv_create(my_port_conf_t *conf, size_t size)
{
	my_port_t *source;

	source = my_mem_alloc(size);
	if (!source) {
		goto _MY_ERR_alloc;
	}

	source->conf = conf;

	return source;

	my_mem_free(source);
_MY_ERR_alloc:
	return NULL;
}

void my_source_priv_destroy(my_port_t *source)
{
	my_mem_free(source);
}

my_port_t *my_source_create(my_port_conf_t *conf)
{
	my_port_t *source;
	my_port_impl_t *impl;

	impl = my_source_impl_find(conf);
	if (!impl) {
		return 0;
	}

	source = impl->create(conf);
	if (!source) {
		my_log(MY_LOG_ERROR, "core/source: error creating source #%d '%s'", conf->index, conf->name);
		return 0;
	}

	MY_PORT_GET_IMPL(source) = impl;

	return source;
}

void my_source_destroy(my_port_t *source)
{
	MY_PORT_GET_IMPL(source)->destroy(source);
}

static int my_source_create_fn(void *data, void *user, int flags)
{
	my_core_t *core = MY_CORE(user);
	my_port_t *source;
	my_port_conf_t *conf = MY_PORT_CONF(data);

	source = my_source_create(conf);
	if (source) {
		my_list_enqueue(core->sources, source);
		source->core = core;
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
	my_port_t *source;

	MY_DEBUG("core/source: destroying all sources");
	while (source = my_list_dequeue(core->sources)) {
		my_source_destroy(source);
	}
}


static int my_source_open_fn(void *data, void *user, int flags)
{
	my_port_t *source = MY_PORT(data);

	MY_PORT_GET_IMPL(source)->open(source);

	return 0;
}

int my_source_open_all(my_core_t *core)
{
	MY_DEBUG("core/source: opening all sources");
	return my_list_iter(core->sources, my_source_open_fn, core);
}


static int my_source_close_fn(void *data, void *user, int flags)
{
	my_port_t *source = MY_PORT(data);

	MY_PORT_GET_IMPL(source)->close(source);

	return 0;
}

int my_source_close_all(my_core_t *core)
{
	MY_DEBUG("core/source: closing all sources");
	return my_list_iter(core->sources, my_source_close_fn, core);
}


void my_source_register(my_port_impl_t *source)
{
	my_list_enqueue(&my_sources, source);
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
