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
#include "core/controls.h"
#include "core/filters.h"
#include "core/sources.h"
#include "core/targets.h"
#include "core/wirings.h"

#include "util/list.h"
#include "util/log.h"
#include "util/mem.h"

static char *my_conf_get_default_name(char *s, int n)
{
	char buf[1024];

	snprintf(buf, sizeof(buf), "%s/%d", s, n);

	return strdup(buf);
}

static int my_conf_parse_controls(my_conf_t *conf, config_setting_t *list)
{
	int i, n;
	config_setting_t *item;
	const char *str_value;
	my_control_conf_t *control;

	n = config_setting_length(list);
	for (i = 0; i < n; i++) {
		item = config_setting_get_elem(list, i);
		control = my_mem_alloc(sizeof(*control));
		if (control == NULL) {
			MY_ERROR("conf: error creating control #%d (%s)", i, strerror(errno));
			goto _MY_ERR_alloc;
		}

		control->index = i;

		if (config_setting_lookup_string(item, "name", &str_value) != CONFIG_FALSE) {
			control->name = strdup(str_value);
		} else {
			control->name = my_conf_get_default_name("control", i);
		}

		if (config_setting_lookup_string(item, "description", &str_value) != CONFIG_FALSE) {
			control->desc = strdup(str_value);
		}

		if (config_setting_lookup_string(item, "type", &str_value) != CONFIG_FALSE) {
			control->type = strdup(str_value);
		}

		if (config_setting_lookup_string(item, "url", &str_value) != CONFIG_FALSE) {
			control->url = strdup(str_value);
		}

		if (my_list_enqueue(conf->controls, control)) {
			MY_ERROR("conf: error queuing control #%d '%s'" , control->index, control->name);
			goto _MY_ERR_queue;
		}
	}

	return 0;

_MY_ERR_queue:
	my_mem_free(control);
_MY_ERR_alloc:
	return -1;
}

static int my_conf_parse_filters(my_conf_t *conf, config_setting_t *list)
{
	int i, n;
	config_setting_t *item;
	const char *str_value;
	my_filter_conf_t *filter;
	

	n = config_setting_length(list);
	for (i = 0; i < n; i++) {
		item = config_setting_get_elem(list, i);
		filter = my_mem_alloc(sizeof(*filter));
		if (filter == NULL) {
			MY_ERROR("conf: error creating filter #%d (%s)", i,strerror(errno));
			goto _MY_ERR_alloc;
		}

		filter->index = i;

		if (config_setting_lookup_string(item, "name", &str_value) != CONFIG_FALSE) {
			filter->name = strdup(str_value);
		} else {
			filter->name = my_conf_get_default_name("filter", i);
		}

		if (config_setting_lookup_string(item, "description", &str_value) != CONFIG_FALSE) {
			filter->desc = strdup(str_value);
		}

		if (config_setting_lookup_string(item, "type", &str_value) != CONFIG_FALSE) {
			filter->type = strdup(str_value);
		}

		if (config_setting_lookup_string(item, "arg", &str_value) != CONFIG_FALSE) {
			filter->arg = strdup(str_value);
		}

		if (my_list_enqueue(conf->filters, filter)) {
			MY_ERROR("conf: error queuing filter #%d '%s'" , filter->index, filter->name);
			goto _MY_ERR_queue;
		}
	}

	return 0;

_MY_ERR_queue:
	my_mem_free(filter);
_MY_ERR_alloc:
	return -1;
}

static int my_conf_parse_sources(my_conf_t *conf, config_setting_t *list)
{
	int i, n;
	config_setting_t *item;
	const char *str_value;
	my_source_conf_t *source;

	n = config_setting_length(list);
	for (i = 0; i < n; i++) {
		item = config_setting_get_elem(list, i);
		source = my_mem_alloc(sizeof(*source));
		if (source == NULL) {
			MY_ERROR("conf: error creating source #%d (%s)", i, strerror(errno));
			goto _MY_ERR_alloc;
		}

		source->index = i;

		if (config_setting_lookup_string(item, "name", &str_value) != CONFIG_FALSE) {
			source->name = strdup(str_value);
		} else {
			source->name = my_conf_get_default_name("source", i);
		}

		if (config_setting_lookup_string(item, "description", &str_value) != CONFIG_FALSE) {
			source->desc = strdup(str_value);
		}

		if (config_setting_lookup_string(item, "type", &str_value) != CONFIG_FALSE) {
			source->type = strdup(str_value);
		}

		if (config_setting_lookup_string(item, "url", &str_value) != CONFIG_FALSE) {
			source->url = strdup(str_value);
		}

		if (my_list_enqueue(conf->sources, source)) {
			MY_ERROR("conf: error queuing source #%d '%s'" , source->index, source->name);
			goto _MY_ERR_queue;
		}
	}

	return 0;

_MY_ERR_queue:
	my_mem_free(source);
_MY_ERR_alloc:
	return -1;
}

static int my_conf_parse_targets(my_conf_t *conf, config_setting_t *list)
{
	int i, n;
	config_setting_t *item;
	const char *str_value;
	my_target_conf_t *target;
	

	n = config_setting_length(list);
	for (i = 0; i < n; i++) {
		item = config_setting_get_elem(list, i);
		target = my_mem_alloc(sizeof(*target));
		if (target == NULL) {
			MY_ERROR("conf: error creating target #%d (%s)", i, strerror(errno));
			goto _MY_ERR_alloc;
		}

		target->index = i;

		if (config_setting_lookup_string(item, "name", &str_value) != CONFIG_FALSE) {
			target->name = strdup(str_value);
		} else {
			target->name = my_conf_get_default_name("target", i);
		}

		if (config_setting_lookup_string(item, "description", &str_value) != CONFIG_FALSE) {
			target->desc = strdup(str_value);
		}

		if (config_setting_lookup_string(item, "type", &str_value) != CONFIG_FALSE) {
			target->type = strdup(str_value);
		}

		if (config_setting_lookup_string(item, "url", &str_value) != CONFIG_FALSE) {
			target->url = strdup(str_value);
		}

		if (my_list_enqueue(conf->targets, target)) {
			MY_ERROR("conf: error queuing target #%d '%s'" , target->index, target->name);
			goto _MY_ERR_queue;
		}
	}

	return 0;

_MY_ERR_queue:
	my_mem_free(target);
_MY_ERR_alloc:
	return -1;
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
