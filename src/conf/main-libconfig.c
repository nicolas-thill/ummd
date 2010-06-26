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

static void my_conf_parse_controls(my_conf_t *conf, config_setting_t *list)
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
			MY_ERROR("error creating control (%s)" , strerror(errno));
			exit(1);
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

		if (my_list_queue(conf->controls, control)) {
			MY_ERROR("error queuing control #d '%s'" , control->index, control->name);
			exit(1);
		}
	}
}

static void my_conf_parse_filters(my_conf_t *conf, config_setting_t *list)
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
			MY_ERROR("error creating filter (%s)" , strerror(errno));
			exit(1);
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

		if (my_list_queue(conf->filters, filter)) {
			MY_ERROR("error queuing filter #d '%s'" , filter->index, filter->name);
			exit(1);
		}
	}
}

static void my_conf_parse_sources(my_conf_t *conf, config_setting_t *list)
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
			MY_ERROR("error creating source (%s)" , strerror(errno));
			exit(1);
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

		if (my_list_queue(conf->sources, source)) {
			MY_ERROR("error queuing source #d '%s'" , source->index, source->name);
			exit(1);
		}
	}
}

static void my_conf_parse_targets(my_conf_t *conf, config_setting_t *list)
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
			MY_ERROR("error creating target (%s)" , strerror(errno));
			exit(1);
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

		if (my_list_queue(conf->targets, target)) {
			MY_ERROR("error queuing target #d '%s'" , target->index, target->name);
			exit(1);
		}
	}
}

static void my_conf_parse_wirings(my_conf_t *conf, config_setting_t *list)
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
			MY_ERROR("error creating wiring (%s)" , strerror(errno));
			exit(1);
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

		if (my_list_queue(conf->wirings, wiring)) {
			MY_ERROR("error queuing wiring #d '%s'" , wiring->index, wiring->name);
			exit(1);
		}
	}
}

my_conf_t *my_conf_create(void)
{
	my_conf_t *conf;

	conf = my_mem_alloc(sizeof(*conf));
	if (!conf) {
		MY_ERROR("error creating config data (%s)" , strerror(errno));
		exit(1);
	}

	conf->controls = my_list_create();
	if (conf->controls == NULL) {
		MY_ERROR("error creating control list (%s)" , strerror(errno));
		exit(1);
	}

	conf->filters = my_list_create();
	if (conf->filters == NULL) {
		MY_ERROR("error creating filter list (%s)" , strerror(errno));
		exit(1);
	}

	conf->sources = my_list_create();
	if (conf->sources == NULL) {
		MY_ERROR("error creating source list (%s)" , strerror(errno));
		exit(1);
	}

	conf->targets = my_list_create();
	if (conf->targets == NULL) {
		MY_ERROR("error creating target list (%s)" , strerror(errno));
		exit(1);
	}

	conf->wirings = my_list_create();
	if (conf->wirings == NULL) {
		MY_ERROR("error creating wiring list (%s)" , strerror(errno));
		exit(1);
	}

	return conf;
}

void my_conf_destroy(my_conf_t *conf)
{
	my_mem_free(conf);
}

void my_conf_parse(my_conf_t *conf)
{
	config_t config;
	config_setting_t *item;
	const char *str_value;
	long int int_value;

	config_init(&config);

	if (config_read_file(&config, conf->cfg_file) == CONFIG_FALSE) {
		MY_ERROR("error reading configuration file '%s' (%s, line %d)", conf->cfg_file, config_error_text(&config), config_error_line(&config));
		exit(1);
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
		my_conf_parse_controls(conf, item);
	}

	item = config_lookup(&config, "filters");
	if (item) {
		my_conf_parse_filters(conf, item);
	}

	item = config_lookup(&config, "sources");
	if (item) {
		my_conf_parse_sources(conf, item);
	}

	item = config_lookup(&config, "targets");
	if (item) {
		my_conf_parse_targets(conf, item);
	}

	item = config_lookup(&config, "wirings");
	if (item) {
		my_conf_parse_wirings(conf, item);
	}

	config_destroy(&config);
}
