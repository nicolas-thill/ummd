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

#include <libconfig.h>

#include "conf.h"

#include "core.h"
#include "core/ports.h"
#include "core/wirings.h"

#include "util/list.h"
#include "util/log.h"
#include "util/mem.h"

static char *my_conf_get_default_name(char *s, int n)
{
	char buf[1024];

	snprintf(buf, sizeof(buf), "%ss[%d]", s, n);

	return strdup(buf);
}

static my_port_conf_t *my_conf_parse_port(config_setting_t *group, char *class)
{
	my_port_conf_t *port_conf;
	int port_index;
	char *port_name;
	const char *prop_name;
	const char *prop_value;
	const config_setting_t *item;
	int i, n;

	port_index = config_setting_index(group);

	if (config_setting_lookup_string(group, "name", &prop_value) != CONFIG_FALSE) {
		port_name = strdup(prop_value);
	} else {
		port_name = my_conf_get_default_name(class, port_index);
	}

	port_conf = my_port_conf_create(port_index, port_name);
	if (port_conf == NULL) {
		goto _MY_ERR_alloc;
	}

	n = config_setting_length(group);
	for (i = 0; i < n; i++) {
		item = config_setting_get_elem(group, i);

		prop_name = config_setting_name(item);
		prop_value = config_setting_get_string(item);
		my_prop_add(port_conf->properties, prop_name, prop_value);
	}

	return port_conf;

	my_mem_free(port_conf);
_MY_ERR_alloc:
	return NULL;
}

static int my_conf_parse_ports(config_setting_t *list, char *class, my_list_t *ports)
{
	int i, n;
	config_setting_t *item;
	my_port_conf_t *port_conf;

	n = config_setting_length(list);
	for (i = 0; i < n; i++) {
		item = config_setting_get_elem(list, i);
		
		port_conf = my_conf_parse_port(item, class);
		if (port_conf == NULL) {
			MY_ERROR("conf: error parsing %s #%d", class, i);
			goto _MY_ERR_create;
		}

		if (my_list_enqueue(ports, port_conf)) {
			MY_ERROR("conf: error queuing %s #%d", class, i);
			goto _MY_ERR_queue;
		}
	}

	return 0;

_MY_ERR_queue:
	my_port_conf_destroy(port_conf);
_MY_ERR_create:
	return -1;
}

static int my_conf_parse_controls(my_conf_t *conf, config_setting_t *list)
{
	return my_conf_parse_ports(list, "control", conf->controls);
}

static int my_conf_parse_filters(my_conf_t *conf, config_setting_t *list)
{
	return my_conf_parse_ports(list, "filter", conf->filters);
}

static int my_conf_parse_sources(my_conf_t *conf, config_setting_t *list)
{
	return my_conf_parse_ports(list, "source", conf->sources);
}

static int my_conf_parse_targets(my_conf_t *conf, config_setting_t *list)
{
	return my_conf_parse_ports(list, "target", conf->targets);
}

static int my_conf_parse_wirings(my_conf_t *conf, config_setting_t *list)
{
	int i, n;
	config_setting_t *item;
	const char *str_value;
	my_wiring_conf_t *wiring;
	

	n = config_setting_length(list);
	for (i = 0; i < n; i++) {
		item = config_setting_get_elem(list, i);
		wiring = my_mem_alloc(sizeof(*wiring));
		if (wiring == NULL) {
			MY_ERROR("conf: error creating wiring #%d (%s)", i, strerror(errno));
			goto _MY_ERR_alloc;
		}

		wiring->index = i;

		if (config_setting_lookup_string(item, "name", &str_value) != CONFIG_FALSE) {
			wiring->name = strdup(str_value);
		} else {
			wiring->name = my_conf_get_default_name("wiring", i);
		}

		if (config_setting_lookup_string(item, "description", &str_value) != CONFIG_FALSE) {
			wiring->desc = strdup(str_value);
		}

		if (config_setting_lookup_string(item, "source", &str_value) != CONFIG_FALSE) {
			wiring->source = strdup(str_value);
		}

		if (config_setting_lookup_string(item, "target", &str_value) != CONFIG_FALSE) {
			wiring->target = strdup(str_value);
		}

		if (my_list_enqueue(conf->wirings, wiring)) {
			MY_ERROR("conf: error queuing wiring #%d '%s'" , wiring->index, wiring->name);
			goto _MY_ERR_queue;
		}
	}

	return 0;

_MY_ERR_queue:
	my_mem_free(wiring);
_MY_ERR_alloc:
	return -1;
}

my_conf_t *my_conf_create(void)
{
	my_conf_t *conf;

	conf = my_mem_alloc(sizeof(*conf));
	if (!conf) {
		MY_ERROR("conf: error creating data (%s)" , strerror(errno));
		goto _MY_ERR_alloc;
	}

	conf->controls = my_list_create();
	if (conf->controls == NULL) {
		MY_ERROR("conf: error creating control list (%s)" , strerror(errno));
		goto _MY_ERR_create_controls;
	}

	conf->filters = my_list_create();
	if (conf->filters == NULL) {
		MY_ERROR("conf: error creating filter list (%s)" , strerror(errno));
		goto _MY_ERR_create_filters;
	}

	conf->sources = my_list_create();
	if (conf->sources == NULL) {
		MY_ERROR("conf: error creating source list (%s)" , strerror(errno));
		goto _MY_ERR_create_sources;
	}

	conf->targets = my_list_create();
	if (conf->targets == NULL) {
		MY_ERROR("conf: error creating target list (%s)" , strerror(errno));
		goto _MY_ERR_create_targets;
	}

	conf->wirings = my_list_create();
	if (conf->wirings == NULL) {
		MY_ERROR("conf: error creating wiring list (%s)" , strerror(errno));
		goto _MY_ERR_create_wirings;
	}

	return conf;

	my_list_destroy(conf->wirings);
_MY_ERR_create_wirings:
	my_list_destroy(conf->targets);
_MY_ERR_create_targets:
	my_list_destroy(conf->sources);
_MY_ERR_create_sources:
	my_list_destroy(conf->filters);
_MY_ERR_create_filters:
	my_list_destroy(conf->controls);
_MY_ERR_create_controls:
	my_mem_free(conf);
_MY_ERR_alloc:
	return NULL;
}


void my_conf_destroy(my_conf_t *conf)
{
	void *data;

	my_list_purge(conf->wirings, MY_LIST_PURGE_FLAG_FREE_DATA);
	my_list_destroy(conf->wirings);

	my_list_purge(conf->targets, MY_LIST_PURGE_FLAG_FREE_DATA);
	my_list_destroy(conf->targets);

	my_list_purge(conf->sources, MY_LIST_PURGE_FLAG_FREE_DATA);
	my_list_destroy(conf->sources);

	my_list_purge(conf->filters, MY_LIST_PURGE_FLAG_FREE_DATA);
	my_list_destroy(conf->filters);

	my_list_purge(conf->controls, MY_LIST_PURGE_FLAG_FREE_DATA);
	my_list_destroy(conf->controls);

	my_mem_free(conf);
}

int my_conf_parse(my_conf_t *conf)
{
	config_t config;
	config_setting_t *item;
	const char *str_value;
	long int int_value;

	config_init(&config);

	if (config_read_file(&config, conf->cfg_file) == CONFIG_FALSE) {
		MY_ERROR("error reading configuration file '%s' (%s, line %d)", conf->cfg_file, config_error_text(&config), config_error_line(&config));
		goto _MY_ERR_read_file;
	}

	if (conf->log_file == NULL) {
		if (config_lookup_string(&config, "log", &str_value) != CONFIG_FALSE) {
			conf->log_file = strdup(str_value);
		}
	}

	if (conf->pid_file == NULL) {
		if (config_lookup_string(&config, "pid-file", &str_value) != CONFIG_FALSE) {
			conf->pid_file = strdup(str_value);
		}
	}

	if (config_lookup_int(&config, "log-level", &int_value) != CONFIG_FALSE) {
		conf->log_level = (int)int_value;
	}

	item = config_lookup(&config, "controls");
	if (item) {
		if (my_conf_parse_controls(conf, item) != 0) {
			goto _MY_ERR_parse_controls;
		}
	}

	item = config_lookup(&config, "filters");
	if (item) {
		if (my_conf_parse_filters(conf, item) != 0) {
			goto _MY_ERR_parse_filters;
		}
	}

	item = config_lookup(&config, "sources");
	if (item) {
		if (my_conf_parse_sources(conf, item) != 0) {
			goto _MY_ERR_parse_sources;
		}
	}

	item = config_lookup(&config, "targets");
	if (item) {
		if (my_conf_parse_targets(conf, item) != 0) {
			goto _MY_ERR_parse_targets;
		}
	}

	item = config_lookup(&config, "wirings");
	if (item) {
		if (my_conf_parse_wirings(conf, item) != 0) {
			goto _MY_ERR_parse_wirings;
		}
	}

	config_destroy(&config);

	return 0;

_MY_ERR_parse_wirings:
_MY_ERR_parse_targets:
_MY_ERR_parse_sources:
_MY_ERR_parse_filters:
_MY_ERR_parse_controls:
_MY_ERR_read_file:
	config_destroy(&config);
	return -1;
}
