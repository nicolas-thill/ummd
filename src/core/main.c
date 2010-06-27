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

#include <event.h>

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
	struct event_base *event_base;
	struct event event_sigint;
	struct event event_sigquit;
	struct event event_sigterm;
};

#define MY_CORE(p) ((my_core_t *)p)
#define MY_CORE_PRIV(p) ((my_core_priv_t *)p)

static void my_core_handle_shutdown(int sig, short event, void *p)
{
	my_log(MY_LOG_NOTICE, "core: received signal %d" , sig);
	event_base_loopexit(MY_CORE_PRIV(p)->event_base, NULL);
}

my_core_t *my_core_create(void)
{
	void *p;

	p = my_mem_alloc(sizeof(my_core_priv_t));
	if (!p) {
		MY_ERROR("core: error creating internal data (%s)" , strerror(errno));
		goto _MY_ERR_alloc;
	}
	
	MY_CORE(p)->controls = my_list_create();
	if (MY_CORE(p)->controls == NULL) {
		MY_ERROR("core: error creating control list (%s)" , strerror(errno));
		goto _MY_ERR_create_controls;
	}

	MY_CORE(p)->filters = my_list_create();
	if (MY_CORE(p)->filters == NULL) {
		MY_ERROR("core: error creating filter list (%s)" , strerror(errno));
		goto _MY_ERR_create_filters;
	}

	MY_CORE(p)->sources = my_list_create();
	if (MY_CORE(p)->sources == NULL) {
		MY_ERROR("core: error creating source list (%s)" , strerror(errno));
		goto _MY_ERR_create_sources;
	}

	MY_CORE(p)->targets = my_list_create();
	if (MY_CORE(p)->targets == NULL) {
		MY_ERROR("core: error creating target list (%s)" , strerror(errno));
		goto _MY_ERR_create_targets;
	}

	if (event_init() == NULL) {
		MY_ERROR("core: error initializing event handling library");
		goto _MY_ERR_event_init;
	}
	
	MY_CORE_PRIV(p)->event_base = event_base_new();
	if (MY_CORE_PRIV(p)->event_base == NULL) {
		MY_ERROR("core: error initializing event handling library");
		goto _MY_ERR_event_base_new;
	}

	event_set(&(MY_CORE_PRIV(p)->event_sigint), SIGINT, EV_SIGNAL, my_core_handle_shutdown, p);
	if (my_core_event_add(MY_CORE(p), &(MY_CORE_PRIV(p)->event_sigint)) != 0) {
		MY_ERROR("core: error installing INT signal handler");
		goto _MY_ERR_add_event_sigint;
	}

	event_set(&(MY_CORE_PRIV(p)->event_sigquit), SIGQUIT, EV_SIGNAL, my_core_handle_shutdown, p);
	if (my_core_event_add(MY_CORE(p), &(MY_CORE_PRIV(p)->event_sigquit)) != 0) {
		MY_ERROR("core: error installing QUIT signal handler");
		goto _MY_ERR_add_event_sigquit;
	}

	event_set(&(MY_CORE_PRIV(p)->event_sigterm), SIGTERM, EV_SIGNAL, my_core_handle_shutdown, p);
	if (my_core_event_add(MY_CORE(p), &(MY_CORE_PRIV(p)->event_sigterm)) != 0 ) {
		MY_ERROR("core: error installing TERM signal handler");
		goto _MY_ERR_add_event_sigterm;
	}

	return MY_CORE(p);

	my_core_event_del(MY_CORE(p), &(MY_CORE_PRIV(p)->event_sigterm));
_MY_ERR_add_event_sigterm:
	my_core_event_del(MY_CORE(p), &(MY_CORE_PRIV(p)->event_sigquit));
_MY_ERR_add_event_sigquit:
	my_core_event_del(MY_CORE(p), &(MY_CORE_PRIV(p)->event_sigint));
_MY_ERR_add_event_sigint:
	event_base_free(MY_CORE_PRIV(p)->event_base);
_MY_ERR_event_base_new:
_MY_ERR_event_init:
	my_list_destroy(MY_CORE(p)->targets);
_MY_ERR_create_targets:
	my_list_destroy(MY_CORE(p)->sources);
_MY_ERR_create_sources:
	my_list_destroy(MY_CORE(p)->filters);
_MY_ERR_create_filters:
	my_list_destroy(MY_CORE(p)->controls);
_MY_ERR_create_controls:
	my_mem_free(p);
_MY_ERR_alloc:
	return NULL;
}

void my_core_destroy(my_core_t *core)
{
	my_control_close_all(core);
	my_control_destroy_all(core);

	my_core_event_del(MY_CORE(core), &(MY_CORE_PRIV(core)->event_sigint));
	my_core_event_del(MY_CORE(core), &(MY_CORE_PRIV(core)->event_sigquit));
	my_core_event_del(MY_CORE(core), &(MY_CORE_PRIV(core)->event_sigterm));

	event_base_free(MY_CORE_PRIV(core)->event_base);

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

	if (my_control_open_all(core) != 0) {
		goto _MY_ERR_open_controls;
	}

	return 0;

	my_control_close_all(core);
_MY_ERR_open_controls:
	my_control_destroy_all(core);
_MY_ERR_create_controls:
	return -1;
}

void my_core_loop(my_core_t *core)
{
	event_base_dispatch(MY_CORE_PRIV(core)->event_base);
}

int my_core_event_add(my_core_t *core, struct event *event)
{
	if (event_base_set(MY_CORE_PRIV(core)->event_base, event) != 0) {
		return -1;
	}
	if (event_add(event, NULL) != 0) {
		return -1;
	}

	return 0;
}

int my_core_event_del(my_core_t *p, struct event *event)
{
	if (event_del(event) != 0) {
		return -1;
	}

	return 0;
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
