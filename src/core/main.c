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
#include "util/list.h"
#include <err.h>

typedef struct my_core_priv my_core_priv_t;

struct my_core_priv {
	my_core_t base;
	int running;
	fd_set watching_fds;
	int max_sock;
	my_list_t *watched_fd_list;
};

struct watch_entry {
	int fd;
	my_event_handler_t callback_fn;
	void *data;
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

static void my_core_watched_fds_update(my_core_t *core)
{
	my_core_priv_t *core_priv = MY_CORE_PRIV(core);
	struct watch_entry *watch_entry;
	my_node_t *node;
	int tmp_max_sock = 0;
	fd_set tmp_watched_fds;

	FD_ZERO(&tmp_watched_fds);

	my_list_for_each(watch_entry, node, core_priv->watched_fd_list) {
		if (watch_entry->fd > tmp_max_sock)
			tmp_max_sock = watch_entry->fd;

		FD_SET(watch_entry->fd, &tmp_watched_fds);
	}

	core_priv->max_sock = tmp_max_sock;
	memcpy(&core_priv->watching_fds, &tmp_watched_fds, sizeof(fd_set));
}

my_core_t *my_core_create(void)
{
	my_core_t *core;
	my_core_priv_t *core_priv;

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

	core_priv = MY_CORE_PRIV(core);

	core_priv->watched_fd_list = my_list_create();
	if (!core_priv->watched_fd_list) {
		MY_ERROR("core: error creating watched fd list (%s)" , strerror(errno));
		goto _MY_ERR_create_watched_fds;
	}

	FD_ZERO(&core_priv->watching_fds);
	core_priv->max_sock = 0;
	return core;

	my_list_destroy(core_priv->watched_fd_list);
_MY_ERR_create_watched_fds:
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
	my_core_priv_t *core_priv = MY_CORE_PRIV(core);

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

	my_list_destroy(core_priv->watched_fd_list);
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
	my_core_priv_t *core_priv = MY_CORE_PRIV(core);
	my_node_t *node;
	struct watch_entry *watch_entry;
	fd_set tmp_watched_fds;
	struct timeval tv;
	int ret, timeout = 1000;

	core_priv->running = 1;

	while (core_priv->running) {

		memcpy(&tmp_watched_fds, &core_priv->watching_fds, sizeof(fd_set));
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;

		ret = select(core_priv->max_sock + 1, &tmp_watched_fds, NULL, NULL, &tv);

		/* timeout occurred */
		if (ret == 0)
			continue;

		if (ret < 0) {
			if (errno != EINTR)
				my_log(MY_LOG_ERROR, "core: select error '%s'", strerror(errno));

			continue;
		}

		my_list_for_each(watch_entry, node, core_priv->watched_fd_list) {

			if (!FD_ISSET(watch_entry->fd, &tmp_watched_fds))
				continue;

			(watch_entry->callback_fn)(watch_entry->fd, watch_entry->data);
		}
	}
}

int my_core_event_handler_add(my_core_t *core, int fd, my_event_handler_t handler, void *p)
{
	my_core_priv_t *core_priv = MY_CORE_PRIV(core);
	struct watch_entry *watch_entry;
	my_node_t *node;

	my_list_for_each(watch_entry, node, core_priv->watched_fd_list) {

		if (watch_entry->fd != fd)
			continue;

		my_log(MY_LOG_ERROR, "core: trying to register an already watched fd: '%i'", fd);
		goto err;
	}

	watch_entry = my_mem_alloc(sizeof(struct watch_entry));
	if (!watch_entry) {
		my_log(MY_LOG_ERROR, "core: error creating register data (%s)" , strerror(errno));
		goto err;
	}

	watch_entry->fd = fd;
	watch_entry->callback_fn = handler;
	watch_entry->data = p;

	my_list_enqueue(core_priv->watched_fd_list, watch_entry);
	my_core_watched_fds_update(core);
	return 0;

err:
	return -1;
}

int my_core_event_handler_del(my_core_t *core, int fd)
{
	my_core_priv_t *core_priv = MY_CORE_PRIV(core);
	struct watch_entry *watch_entry;
	my_node_t *node;

	my_list_for_each(watch_entry, node, core_priv->watched_fd_list) {
		if (watch_entry->fd == fd)
			break;

		watch_entry = NULL;
	}

	if (!watch_entry) {
		my_log(MY_LOG_ERROR, "core: error unregistering fd: unknown fd (%i)" , fd);
		goto err;
	}

	my_list_remove(node);
	my_mem_free(watch_entry);
	my_core_watched_fds_update(core);
	return 0;

err:
	return -1;
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
