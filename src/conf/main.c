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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "conf.h"

#include "core.h"
#include "core/ports.h"
#include "core/wirings.h"

#include "util/list.h"
#include "util/log.h"

#ifdef MY_DEBUGGING

static int my_conf_dump_port_properties_fn(char *name, char *value, void *user, int flags)
{
	MY_DEBUG("\t\t%s=\"%s\";", name, value);
}

static int my_conf_dump_port_fn(void *data, void *user, int flags)
{
	my_port_conf_t *port = (my_port_conf_t *)data;
	char *class = (char *)user;

	MY_DEBUG("\t%s #%d = {", class, port->index);
	MY_DEBUG("\t\tname=\"%s\";", port->name);
	my_prop_iter(port->properties, my_conf_dump_port_properties_fn, NULL);
	MY_DEBUG("\t}%s", flags & MY_LIST_ITER_FLAG_LAST ? "" : ",");

	return 0;
}

static int my_conf_dump_wiring_fn(void *data, void *user, int flags)
{
	my_wiring_conf_t *wiring = (my_wiring_conf_t *)data;

	MY_DEBUG("\twiring #%d = {", wiring->index);
	MY_DEBUG("\t\tname=\"%s\";", wiring->name);
	MY_DEBUG("\t\tsource=\"%s\";", wiring->source);
	MY_DEBUG("\t\ttarget=\"%s\";", wiring->target);
	MY_DEBUG("\t}%s", flags & MY_LIST_ITER_FLAG_LAST ? "" : ",");

	return 0;
}

void my_conf_dump(my_conf_t *conf)
{
	MY_DEBUG("cfg-file = \"%s\";", conf->cfg_file);
	MY_DEBUG("log-file = \"%s\";", conf->log_file);
	MY_DEBUG("log-level = %d;", conf->log_level);
	MY_DEBUG("pid-file = \"%s\";", conf->pid_file);

	MY_DEBUG("controls = (");
	my_list_iter(conf->controls, my_conf_dump_port_fn, "control");
	MY_DEBUG(");");

	MY_DEBUG("filters = (");
	my_list_iter(conf->filters, my_conf_dump_port_fn, "filter");
	MY_DEBUG(");");

	MY_DEBUG("sources = (");
	my_list_iter(conf->sources, my_conf_dump_port_fn, "source");
	MY_DEBUG(");");

	MY_DEBUG("targets = (");
	my_list_iter(conf->targets, my_conf_dump_port_fn, "target");
	MY_DEBUG(");");

	MY_DEBUG("wirings = (");
	my_list_iter(conf->wirings, my_conf_dump_wiring_fn, NULL);
	MY_DEBUG(");");
}

#endif /* MY_DEBUGGING */
