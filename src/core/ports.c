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
#include "util/mem.h"

my_port_conf_t *my_port_conf_create(int port_index, char *port_name)
{
	my_port_conf_t *port_conf;

	port_conf = my_mem_alloc(sizeof(*port_conf));
	if (port_conf == NULL) {
		goto _MY_ERR_alloc;
	}

	port_conf->properties = my_list_create();
	if (port_conf->properties == NULL) {
		goto _MY_ERR_create_properties;
	}

	port_conf->index = port_index;
	port_conf->name = port_name;

	return port_conf;

_MY_ERR_create_properties:
	my_mem_free(port_conf);
_MY_ERR_alloc:
	return NULL;
}

void my_port_conf_destroy(my_port_conf_t *port_conf)
{
	my_list_destroy(port_conf->properties);
	my_mem_free(port_conf);
}


my_port_impl_t *my_port_impl_lookup(my_list_t *list, char *name)
{
	my_port_impl_t *impl;
	my_node_t *node;

	for (node = list->head; node; node = node->next) {
		impl = MY_PORT_IMPL(node->data);
		if (strcmp(impl->name, name) == 0) {
			return impl;
		}
	}

	return NULL;
}

void my_port_impl_register(my_list_t *list, my_port_impl_t *impl)
{
	my_list_enqueue(list, impl);
}


my_port_t *my_port_create_priv(int size)
{
	my_port_t *port;

	port = my_mem_alloc(size);
	if (!port) {
		goto _MY_ERR_alloc;
	}

	return port;

	my_mem_free(port);
_MY_ERR_alloc:
	return NULL;
}

void my_port_destroy_priv(my_port_t *port)
{
	my_mem_free(port);
}


my_port_t *my_port_create(my_core_t *core, my_port_conf_t *conf, my_port_impl_t *impl)
{
	my_port_t *port;

	port = impl->create(core, conf);
	if (!port) {
		return NULL;
	}

	port->core = core;
	port->conf = conf;
	port->impl = impl;

	return port;
}

void my_port_destroy(my_port_t *port)
{
	MY_PORT_GET_IMPL(port)->destroy(port);
}


my_port_t *my_port_lookup_by_name(my_list_t *list, char *name)
{
	my_node_t *node;
	my_port_t *port;

	for (node = list->head; node; node = node->next) {
		port = MY_PORT(node->data);
		if (strcmp(port->conf->name, name) == 0) {
			return port;
		}
	}

	return NULL;
}


int my_port_destroy_all(my_list_t *list)
{
	my_port_t *port;

	while (port = my_list_dequeue(list)) {
		my_port_destroy(port);
	}
}

static int my_port_open_fn(void *data, void *user, int flags)
{
	my_port_t *port = MY_PORT(data);

	MY_PORT_GET_IMPL(port)->open(port);

	return 0;
}

int my_port_open_all(my_list_t *list)
{
	return my_list_iter(list, my_port_open_fn, NULL);
}


static int my_port_close_fn(void *data, void *user, int flags)
{
	my_port_t *port = MY_PORT(data);

	return MY_PORT_GET_IMPL(port)->close(port);

	return 0;
}

int my_port_close_all(my_list_t *list)
{
	return my_list_iter(list, my_port_close_fn, NULL);
}

void my_port_link(my_port_t *port, my_port_t *peer)
{
	if (port) {
		MY_DPORT(port)->peer = MY_PORT(MY_DPORT(peer));
	}
	if (peer) {
		MY_DPORT(peer)->peer = MY_PORT(MY_DPORT(port));
	}
}

int my_port_get(my_port_t *port, void *buf, int len)
{
	return MY_PORT_GET_IMPL(port)->get(port, buf, len);
}

int my_port_put(my_port_t *port, void *buf, int len)
{
	return MY_PORT_GET_IMPL(port)->put(port, buf, len);
}

