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

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/times.h>

#include "core.h"

#include "util/log.h"
#include "util/mem.h"
#include "util/list.h"

#define EVENT_LIST_RESOLUTION 250000	/* microseconds */

typedef struct my_core_priv my_core_priv_t;

struct my_core_priv {
	my_core_t base;
	int running;
	fd_set watching_fds;
	int max_sock;
	int64_t curr_time;
	my_list_t *watched_fd_list;
	my_list_t *alarm_list;
	uint32_t system_tick;
};

struct watch_entry {
	int fd;
	my_event_handler_t callback_fn;
	void *data;
};

struct alarm_entry {
	uint64_t alarm_time;
	uint64_t reoccurring;
	my_alarm_handler_t callback_fn;
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

	my_list_for_each(core_priv->watched_fd_list, node, watch_entry) {
		if (watch_entry->fd > tmp_max_sock)
			tmp_max_sock = watch_entry->fd;

		FD_SET(watch_entry->fd, &tmp_watched_fds);
	}

	core_priv->max_sock = tmp_max_sock;
	memcpy(&core_priv->watching_fds, &tmp_watched_fds, sizeof(fd_set));
}

/* Make times(2) behave rationally on Linux */
static clock_t my_core_times_wrapper(void)
{
	struct tms dummy_tms_struct;
	int save_errno = errno;
	clock_t ret;

	/**
	 * times(2) really returns an unsigned value ...
	 *
	 * We don't check to see if we got back the error value (-1), because
	 * the only possibility for an error would be if the address of
	 * dummy_tms_struct was invalid.  Since it's a
	 * compiler-generated address, we assume that errors are impossible.
	 * And, unfortunately, it is quite possible for the correct return
	 * from times(2) to be exactly (clock_t)-1.  Sigh...
	 *
	 */
	errno = 0;
	ret = times(&dummy_tms_struct);

	/**
	 * This is to work around a bug in the system call interface
	 * for times(2) found in glibc on Linux (and maybe elsewhere)
	 * It changes the return values from -1 to -4096 all into
	 * -1 and then dumps the -(return value) into errno.
	 *
	 * This totally bizarre behavior seems to be widespread in
	 * versions of Linux and glibc.
	 *
	 * Many thanks to Wolfgang Dumhs <wolfgang.dumhs (at) gmx.at>
	 * for finding and documenting this bizarre behavior.
	 */
	if (errno != 0) {
		ret = (clock_t) (-errno);
	}

	errno = save_errno;
	return ret;
}

static uint64_t my_core_get_time_msec(my_core_t *core)
{
	my_core_priv_t *core_priv = MY_CORE_PRIV(core);

	return (my_core_times_wrapper() * 1000) / core_priv->system_tick;
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

	core->wirings = my_list_create();
	if (core->wirings == NULL) {
		MY_ERROR("core: error creating wiring list (%s)" , strerror(errno));
		goto _MY_ERR_create_wirings;
	}

	core_priv = MY_CORE_PRIV(core);

	core_priv->watched_fd_list = my_list_create();
	if (!core_priv->watched_fd_list) {
		MY_ERROR("core: error creating watched fd list (%s)" , strerror(errno));
		goto _MY_ERR_create_watched_fds;
	}

	core_priv->alarm_list = my_list_create();
	if (!core_priv->alarm_list) {
		MY_ERROR("core: error creating alarm list (%s)" , strerror(errno));
		goto _MY_ERR_create_alarm_list;
	}

	FD_ZERO(&core_priv->watching_fds);
	core_priv->max_sock = 0;
	core_priv->system_tick = sysconf(_SC_CLK_TCK);
	core_priv->curr_time = my_core_get_time_msec(core);

	my_audio_codec_init();

	my_control_register_all();
	my_filter_register_all();
	my_source_register_all();
	my_target_register_all();


	return core;

	my_list_destroy(core_priv->alarm_list);
_MY_ERR_create_alarm_list:
	my_list_destroy(core_priv->watched_fd_list);
_MY_ERR_create_watched_fds:
	my_list_destroy(core->wirings);
_MY_ERR_create_wirings:
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

	my_audio_codec_fini();

	my_target_close_all(core);
	my_source_close_all(core);
	my_control_close_all(core);

	my_wiring_destroy_all(core);
	my_target_destroy_all(core);
	my_source_destroy_all(core);
	my_filter_destroy_all(core);
	my_control_destroy_all(core);

	my_list_destroy(core->controls);
	my_list_destroy(core->filters);
	my_list_destroy(core->sources);
	my_list_destroy(core->targets);
	my_list_destroy(core->wirings);

	my_list_destroy(core_priv->watched_fd_list);
	my_list_purge(core_priv->alarm_list, MY_LIST_PURGE_FLAG_FREE_DATA);
	my_list_destroy(core_priv->alarm_list);
	my_mem_free(core);
}

int my_core_init(my_core_t *core, my_conf_t *conf)
{
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

	if (my_wiring_create_all(core, conf) != 0) {
		goto _MY_ERR_create_wirings;
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
	my_wiring_destroy_all(core);
_MY_ERR_create_wirings:
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

static void my_core_alarm_add_at_pos(my_core_t *core, struct alarm_entry *alarm_entry)
{
	my_core_priv_t *core_priv = MY_CORE_PRIV(core);
	struct alarm_entry *alarm_entry_tmp;
	my_node_t *node;

	my_list_for_each(core_priv->alarm_list, node, alarm_entry_tmp) {

		if ((int64_t)(alarm_entry_tmp->alarm_time - alarm_entry->alarm_time) > 0)
			continue;

		my_list_insert_before(core_priv->alarm_list, node, alarm_entry);
		return;
	}

	my_list_enqueue(core_priv->alarm_list, alarm_entry);
}

static void my_core_alarm_list_add_reoccurring(my_core_t *core, struct alarm_entry *alarm_entry)
{
	my_core_priv_t *core_priv = MY_CORE_PRIV(core);

	alarm_entry->alarm_time = core_priv->curr_time + alarm_entry->reoccurring;
	my_core_alarm_add_at_pos(core, alarm_entry);
}

int my_core_alarm_add(my_core_t *core, unsigned int timeout,
		      int reoccurring, my_alarm_handler_t handler, void *p)
{
	my_core_priv_t *core_priv = MY_CORE_PRIV(core);
	struct alarm_entry *alarm_entry;

	alarm_entry = my_mem_alloc(sizeof(struct alarm_entry));
	if (!alarm_entry) {
		my_log(MY_LOG_ERROR, "core: error creating alarm entry (%s)" , strerror(errno));
		goto err;
	}

	alarm_entry->alarm_time = my_core_get_time_msec(core) + timeout;
	alarm_entry->callback_fn = handler;
	alarm_entry->data = p;

	if (reoccurring)
		alarm_entry->reoccurring = timeout;
	else
		alarm_entry->reoccurring = 0;

	my_core_alarm_add_at_pos(core, alarm_entry);

out:
	return 0;

err:
	return -1;
}

static void my_core_alarm_list_maintain(my_core_t *core)
{
	my_core_priv_t *core_priv = MY_CORE_PRIV(core);
	struct alarm_entry *alarm_entry;
	my_node_t *node;

	my_list_for_each(core_priv->alarm_list, node, alarm_entry) {

		if ((int)(alarm_entry->alarm_time - core_priv->curr_time) > 0)
			break;

		(alarm_entry->callback_fn)(alarm_entry->data);

		my_list_remove(core_priv->alarm_list, node);

		if (alarm_entry->reoccurring)
			my_core_alarm_list_add_reoccurring(core, alarm_entry);
		else
			my_mem_free(alarm_entry);
	}
}

void my_core_loop(my_core_t *core)
{
	my_core_priv_t *core_priv = MY_CORE_PRIV(core);
	my_node_t *node;
	struct watch_entry *watch_entry;
	fd_set tmp_watched_fds;
	struct timeval tv;
	int ret;

	core_priv->running = 1;
	tv.tv_sec = 0;
	tv.tv_usec = EVENT_LIST_RESOLUTION;

	while (core_priv->running) {

		memcpy(&tmp_watched_fds, &core_priv->watching_fds, sizeof(fd_set));

		ret = select(core_priv->max_sock + 1, &tmp_watched_fds, NULL, NULL, &tv);

		core_priv->curr_time = my_core_get_time_msec(core);
		my_core_alarm_list_maintain(core);

		if (ret < 0) {
			if (errno != EINTR)
				my_log(MY_LOG_ERROR, "core: select error '%s'", strerror(errno));
		}

		if (ret <= 0)
			goto reset_sleep;

		my_list_for_each(core_priv->watched_fd_list, node, watch_entry) {

			if (!FD_ISSET(watch_entry->fd, &tmp_watched_fds))
				continue;

			(watch_entry->callback_fn)(watch_entry->fd, watch_entry->data);
		}

		continue;

reset_sleep:
		tv.tv_sec = 0;
		tv.tv_usec = EVENT_LIST_RESOLUTION;
	}
}

void my_core_stop(my_core_t *core)
{
	MY_CORE_PRIV(core)->running = 0;
}

int my_core_event_handler_add(my_core_t *core, int fd, my_event_handler_t handler, void *p)
{
	my_core_priv_t *core_priv = MY_CORE_PRIV(core);
	struct watch_entry *watch_entry;
	my_node_t *node;

	my_list_for_each(core_priv->watched_fd_list, node, watch_entry) {

		if (watch_entry->fd != fd)
			continue;

		my_log(MY_LOG_ERROR, "core: trying to register an already watched fd: '%i'", fd);
		goto err;
	}

	watch_entry = my_mem_alloc(sizeof(struct watch_entry));
	if (!watch_entry) {
		my_log(MY_LOG_ERROR, "core: error creating event handler (%s)" , strerror(errno));
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

	my_list_for_each(core_priv->watched_fd_list, node, watch_entry) {
		if (watch_entry->fd == fd)
			break;

		watch_entry = NULL;
	}

	if (!watch_entry) {
		my_log(MY_LOG_ERROR, "core: error unregistering fd: unknown fd (%i)" , fd);
		goto err;
	}

	my_list_remove(core_priv->watched_fd_list, node);
	my_mem_free(watch_entry);
	my_core_watched_fds_update(core);
	return 0;

err:
	return -1;
}

static char my_proto[] = "UMMD/" VERSION;

int my_core_handle_command(my_core_t *core, void *buf, int len)
{
	char *p = buf;
	int n;

	n = strlen(my_proto);
	if (strncmp(p, my_proto, n) != 0) {
		my_log(MY_LOG_NOTICE, "core: unknown command signature");
	}

	p += n + 1;

	if (strcmp(p, "QUIT") == 0) {
		MY_DEBUG("core: received '%s' command", p);
		my_core_exit(core);
	} else {
		my_log(MY_LOG_ERROR, "core: unknown command '%s'", p);
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
