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

#include "core/wirings.h"

#include "util/list.h"
#include "util/log.h"
#include "util/mem.h"

static my_port_t *my_wiring_source_lookup(my_core_t *core, char *name)
{
	my_port_t *port;

	port = my_port_lookup_by_name(core->sources, name);
	if (!port) {
		port = my_port_lookup_by_name(core->filters, name);
	}

	return port;
}

static my_port_t *my_wiring_target_lookup(my_core_t *core, char *name)
{
	my_port_t *port;

	port = my_port_lookup_by_name(core->targets, name);
	if (!port) {
		port = my_port_lookup_by_name(core->filters, name);
	}

	return port;
}

static my_wiring_t *my_wiring_priv_create(my_core_t *core, my_wiring_conf_t *conf)
{
	my_wiring_t *wiring;
	my_port_t *source, *target;

	source = my_wiring_source_lookup(core, conf->source);
	if (source == NULL) {
		goto _MY_ERR_source_lookup;
	}
	target = my_wiring_target_lookup(core, conf->target);
	if (target == NULL) {
		goto _MY_ERR_target_lookup;
	}

	wiring = my_mem_alloc(sizeof(*wiring));
	if (!wiring) {
		goto _MY_ERR_wiring_alloc;
	}

	wiring->core = core;
	wiring->source = source;
	wiring->target = target;

	my_port_link(source, target);

	return wiring;

_MY_ERR_wiring_alloc:
_MY_ERR_target_lookup:
_MY_ERR_source_lookup:
	return NULL;
}

static void my_wiring_priv_destroy(my_wiring_t *wiring)
{
	my_mem_free(wiring);
}

static int my_wiring_create_fn(void *data, void *user, int flags)
{
	my_core_t *core = MY_CORE(user);
	my_wiring_conf_t *conf = MY_WIRING_CONF(data);
	my_wiring_t *wiring;

	wiring = my_wiring_priv_create(core, conf);
	if (wiring) {
		my_list_enqueue(core->wirings, wiring);
	}

	return 0;
}

int my_wiring_create_all(my_core_t *core, my_conf_t *conf)
{
	MY_DEBUG("core/wirings: creating all");
	return my_list_iter(conf->wirings, my_wiring_create_fn, core);
}

void my_wiring_destroy_all(my_core_t *core)
{
	my_wiring_t *wiring;

	while (wiring = my_list_dequeue(core->wirings)) {
		my_wiring_priv_destroy(wiring);
	}
}
