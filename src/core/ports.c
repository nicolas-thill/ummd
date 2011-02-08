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


my_port_t *my_port_priv_create(my_port_conf_t *conf, int size)
{
	my_port_t *port;

	port = my_mem_alloc(size);
	if (!port) {
		goto _MY_ERR_alloc;
	}

	port->conf = conf;

	return port;

	my_mem_free(port);
_MY_ERR_alloc:
	return NULL;
}

void my_port_priv_destroy(my_port_t *port)
{
	my_mem_free(port);
}


my_port_t *my_port_create(my_port_conf_t *conf, my_port_impl_t *impl)
{
	my_port_t *port;

	port = impl->create(conf);
	if (!port) {
		return NULL;
	}

	MY_PORT_GET_IMPL(port) = impl;

	return port;
}

void my_port_destroy(my_port_t *port)
{
	MY_PORT_GET_IMPL(port)->destroy(port);
}

