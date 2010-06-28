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

#include "core/sources.h"

#include "util/list.h"
#include "util/log.h"
#include "util/mem.h"

static my_list_t my_sources;

#define MY_SOURCE_REGISTER(x) { \
	extern my_source_impl_t my_source_##x; \
	my_source_register(&my_source_##x); \
}

static my_source_impl_t *my_source_find_impl(char *name)
{
	my_source_impl_t *impl;
	my_node_t *node;

	for (node = my_sources.head; node; node = node->next) {
		impl = (my_source_impl_t *)(node->data);
		if (strcmp(impl->name, name) == 0) {
			return impl;
		}
	}

	return NULL;
}

static my_source_t *my_source_create(my_core_t *core, my_source_conf_t *conf)
{
	my_source_t *source;
	my_source_impl_t *impl;
	
	impl = my_source_find_impl(conf->type);
	if (!impl) {
		my_log(MY_LOG_ERROR, "core/source: unknown type '%s' for source #%d '%s'", conf->type, conf->index, conf->name);
		return NULL;
	}
	source = impl->create(conf);
	if (!source) {
		my_log(MY_LOG_ERROR, "core/source: error creating source #%d'%s'", conf->index, conf->name);
		return NULL;
	}

	source->core = core;
	source->conf = conf;
	source->impl = impl;

	return source;
}

static void my_source_destroy(my_source_t *source)
{
	source->impl->destroy(source);
}

static int my_source_create_fn(void *data, void *user, int flags)
{
	my_core_t *core = MY_CORE(user);
	my_source_t *source;
	my_source_conf_t *conf = MY_SOURCE_CONF(data);

	source = my_source_create(core, conf);
	if (source) {
		my_list_enqueue(core->sources, source);
	}

	return 0;
}

static int my_source_open_fn(void *data, void *user, int flags)
{
	my_source_t *source = MY_SOURCE(data);

	source->impl->open(source);

	return 0;
}

static int my_source_close_fn(void *data, void *user, int flags)
{
	my_source_t *source = MY_SOURCE(data);

	source->impl->close(source);

	return 0;
}

int my_source_create_all(my_core_t *core, my_conf_t *conf)
{
	MY_DEBUG("core/source: creating all sources");
	return my_list_iter(conf->sources, my_source_create_fn, core);
}

int my_source_destroy_all(my_core_t *core)
{
	my_source_t *source;

	MY_DEBUG("core/source: destroying all sources");
	while (source = my_list_dequeue(core->sources)) {
		my_source_destroy(source);
	}
}

int my_source_open_all(my_core_t *core)
{
	MY_DEBUG("core/source: opening all sources");
	return my_list_iter(core->sources, my_source_open_fn, core);
}

int my_source_close_all(my_core_t *core)
{
	MY_DEBUG("core/source: closing all sources");
	return my_list_iter(core->sources, my_source_close_fn, core);
}

static void my_source_register(my_source_impl_t *source)
{
	my_list_enqueue(&my_sources, source);
}

void my_source_register_all(void)
{
/*
	MY_SOURCE_REGISTER(core, file);
	MY_SOURCE_REGISTER(core, sock);
	MY_SOURCE_REGISTER(core, net_http_client);
	MY_SOURCE_REGISTER(core, net_rtp_client);
	MY_SOURCE_REGISTER(core, net_udp_client);
*/
}

#ifdef MY_DEBUGGING

static int my_source_dump_fn(void *data, void *user, int flags)
{
	my_source_impl_t *impl = MY_SOURCE_IMPL(data);
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
