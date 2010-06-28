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
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <event.h>

#include "core/controls.h"

#include "util/log.h"
#include "util/mem.h"

typedef struct my_control_priv my_control_priv_t;

struct my_control_priv {
	my_control_t base;
	char *path;
	int fd;
	struct event event;
};

#define MY_CONTROL_PRIV(p) ((my_control_priv_t *)(p))

#define MY_CONTROL_BUF_SIZE  1024

static void my_control_fifo_event_handler(int fd, short event, void *p)
{
	my_control_t *control = (my_control_t *)p;
	char buf[MY_CONTROL_BUF_SIZE + 1];
	int n;

	n = read(fd, buf, MY_CONTROL_BUF_SIZE);
	if (n == -1) {
		my_log(MY_LOG_ERROR, "core/control/fifo: error reading from fifo '%s' (%s)", MY_CONTROL_PRIV(control)->path, strerror(errno));
	}
	buf[MY_CONTROL_BUF_SIZE] = '\0';
	if (n > 0) {
		if (buf[n - 1] == '\n') {
			buf[n - 1] = '\0';
		}
		my_log(MY_LOG_ERROR, "core/control/fifo: received '%s' from fifo '%s'", buf, MY_CONTROL_PRIV(control)->path);
	}
}

static my_control_t *my_control_fifo_create(my_control_conf_t *conf)
{
	my_control_t *control;
	char *p;
	int n;

	control = my_mem_alloc(sizeof(my_control_priv_t));
	if (!control) {
		goto _MY_ERR_alloc;
	}

	p = strchr(conf->url, ':');
	if (p) {
		n = p - conf->url;
		if ((strncmp(conf->url, "file", n) != 0) && (strncmp(conf->url, "fifo", n) != 0)) {
			my_log(MY_LOG_ERROR, "core/control/fifo: unknown method '%.2$*1$s' in url '%$1s'", conf->url, n);
			goto _MY_ERR_parse_url;
		}
		p++;
	} else {
		p = conf->url;
	}

	MY_CONTROL_PRIV(control)->path = p;

	return control;

_MY_ERR_parse_url:
	my_mem_free(control);
_MY_ERR_alloc:
	return NULL;
}

static void my_control_fifo_destroy(my_control_t *control)
{
	my_mem_free(control);
}

static int my_control_fifo_open(my_control_t *control)
{
	MY_DEBUG("core/control/fifo: creating fifo '%s'", MY_CONTROL_PRIV(control)->path);
	if (mkfifo(MY_CONTROL_PRIV(control)->path, 0600) == -1) {
		my_log(MY_LOG_ERROR, "core/control/fifo: error creating fifo '%s' (%s)", MY_CONTROL_PRIV(control)->path, strerror(errno));
		goto _MY_ERR_create_fifo;
	}

	MY_DEBUG("core/control/fifo: opening fifo '%s'", MY_CONTROL_PRIV(control)->path);
	MY_CONTROL_PRIV(control)->fd = open(MY_CONTROL_PRIV(control)->path, O_RDWR | O_NONBLOCK, 0);
	if (MY_CONTROL_PRIV(control)->fd == -1) {
		my_log(MY_LOG_ERROR, "core/control/fifo: error opening fifo '%s' (%s)", MY_CONTROL_PRIV(control)->path, strerror(errno));
		goto _MY_ERR_open_fifo;
	}

	event_set(&(MY_CONTROL_PRIV(control)->event), MY_CONTROL_PRIV(control)->fd, EV_READ | EV_PERSIST, my_control_fifo_event_handler, control);
	my_core_event_add(control->core, &(MY_CONTROL_PRIV(control)->event));
	
	return 0;

_MY_ERR_open_fifo:
_MY_ERR_create_fifo:
	return -1;
}

static int my_control_fifo_close(my_control_t *control)
{
	my_core_event_del(control->core, &(MY_CONTROL_PRIV(control)->event));

	MY_DEBUG("core/control/fifo: closing fifo '%s'", MY_CONTROL_PRIV(control)->path);
	if (close(MY_CONTROL_PRIV(control)->fd) == -1) {
		my_log(MY_LOG_ERROR, "core/control/fifo: error closing fifo '%s' (%s)", MY_CONTROL_PRIV(control)->path, strerror(errno));
	}

	MY_DEBUG("core/control/fifo: removing fifo '%s'", MY_CONTROL_PRIV(control)->path);
	if (unlink(MY_CONTROL_PRIV(control)->path) == -1) {
		my_log(MY_LOG_ERROR, "core/control/fifo: error removing fifo '%s' (%s)", MY_CONTROL_PRIV(control)->path, strerror(errno));
	}

	return 0;
}

my_control_impl_t my_control_fifo = {
	.id = MY_CONTROL_FIFO,
	.name = "fifo",
	.desc = "FIFO (named pipe) control interface",
	.create = my_control_fifo_create,
	.destroy = my_control_fifo_destroy,
	.open = my_control_fifo_open,
	.close = my_control_fifo_close,
};
