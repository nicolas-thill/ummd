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
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

#include "core/controls.h"
#include "core/filters.h"
#include "core/sources.h"
#include "core/targets.h"

#include "util/log.h"
#include "util/mem.h"

typedef struct my_core_priv my_core_priv_t;

struct my_core_priv {
	my_core_t base;
	int running;
};

#define MY_CORE_PRIV(p) ((my_core_priv_t *)p)

static void my_core_exit(my_core_t *core)
{
	MY_CORE_PRIV(core)->running = 0;
}

static void my_core_handle_shutdown(int sig, short event, void *p)
{
	my_log(MY_LOG_NOTICE, "core: received signal %d" , sig);
	my_core_exit(MY_CORE(p));
}

my_core_t *my_core_create(void)
{
	my_core_t *core;

	core = my_mem_alloc(sizeof(my_core_priv_t));
	if (!core) {
		MY_ERROR("core: error creating internal data (%s)" , strerror(errno));
		goto _MY_ERR_alloc;
	}
	
	core->controls = my_list_create();
	if (core->controls == NULL) {
		MY_ERROR("core: error creating control list (%s)" , strerror(errno));
		goto _MY_ERR_create_controls;
	}

	core->filters = my_list_create();
	if (core->filters == NULL) {
		MY_ERROR("core: error creating filter list (%s)" , strerror(errno));
		goto _MY_ERR_create_filters;
	}

	core->sources = my_list_create();
	if (core->sources == NULL) {
		MY_ERROR("core: error creating source list (%s)" , strerror(errno));
		goto _MY_ERR_create_sources;
	}

	core->targets = my_list_create();
	if (core->targets == NULL) {
		MY_ERROR("core: error creating target list (%s)" , strerror(errno));
		goto _MY_ERR_create_targets;
	}

	return core;

	my_list_destroy(core->targets);
_MY_ERR_create_targets:
	my_list_destroy(core->sources);
_MY_ERR_create_sources:
	my_list_destroy(core->filters);
_MY_ERR_create_filters:
	my_list_destroy(core->controls);
_MY_ERR_create_controls:
	my_mem_free(core);
_MY_ERR_alloc:
	return NULL;
}

void my_core_destroy(my_core_t *core)
{
	my_target_close_all(core);
	my_source_close_all(core);
	my_control_close_all(core);

	my_target_destroy_all(core);
	my_source_destroy_all(core);
	my_filter_destroy_all(core);
	my_control_destroy_all(core);

	my_list_destroy(core->controls);
	my_list_destroy(core->filters);
	my_list_destroy(core->sources);
	my_list_destroy(core->targets);

	my_mem_free(core);
}

int my_core_init(my_core_t *core, my_conf_t *conf)
{
	my_control_register_all();
	my_filter_register_all();
	my_source_register_all();
	my_target_register_all();

	if (my_control_create_all(core, conf) != 0) {
		goto _MY_ERR_create_controls;
	}

	if (my_filter_create_all(core, conf) != 0) {
		goto _MY_ERR_create_filters;
	}

	if (my_source_create_all(core, conf) != 0) {
		goto _MY_ERR_create_sources;
	}

	if (my_target_create_all(core, conf) != 0) {
		goto _MY_ERR_create_targets;
	}

	if (my_control_open_all(core) != 0) {
		goto _MY_ERR_open_controls;
	}

	if (my_source_open_all(core) != 0) {
		goto _MY_ERR_open_sources;
	}

	if (my_target_open_all(core) != 0) {
		goto _MY_ERR_open_targets;
	}

	return 0;

	my_target_close_all(core);
_MY_ERR_open_targets:
	my_source_close_all(core);
_MY_ERR_open_sources:
	my_control_close_all(core);
_MY_ERR_open_controls:
	my_target_destroy_all(core);
_MY_ERR_create_targets:
	my_source_destroy_all(core);
_MY_ERR_create_sources:
	my_filter_destroy_all(core);
_MY_ERR_create_filters:
	my_control_destroy_all(core);
_MY_ERR_create_controls:
	return -1;
}

void my_core_loop(my_core_t *core)
{
	MY_CORE_PRIV(core)->running = 1;
	while (MY_CORE_PRIV(core)->running) {
		/* do something */
		sleep(1);
	}
}

int my_core_event_handler_add(my_core_t *core, int fd, my_event_handler_t handler, void *p)
{
	/* do something */

	return 0;
}

int my_core_event_handler_del(my_core_t *p, int fd)
{
	/* do something */

	return 0;
}

int my_core_handle_command(my_core_t *core, char *command)
{
	if (strcmp(command, "/quit") == 0) {
		my_log(MY_LOG_NOTICE, "core: received '%s' command", command);
		my_core_exit(core);
	} else {
		my_log(MY_LOG_ERROR, "core: unknown command '%s'", command);
	}
}

#ifdef MY_DEBUGGING

void my_core_dump(my_core_t *core)
{
	my_control_dump_all();
	my_filter_dump_all();
	my_source_dump_all();
	my_target_dump_all();
}

#endif /* MY_DEBUGGING */
