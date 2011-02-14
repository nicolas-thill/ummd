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

#include "core/ports.h"

#include "util/log.h"
#include "util/mem.h"
#include "util/prop.h"

typedef struct my_control_priv_s my_control_priv_t;

struct my_control_priv_s {
	my_cport_t _inherited;
	char *path;
	int fd;
};

#define MY_CONTROL(p) ((my_control_priv_t *)(p))
#define MY_CONTROL_SIZE (sizeof(my_control_priv_t))


#define MY_CONTROL_BUF_SIZE  255

static int my_control_fifo_event_handler(int fd, void *p)
{
	my_port_t *port = (my_port_t *)p;
	char buf[MY_CONTROL_BUF_SIZE + 1];
	int n;

	n = read(fd, buf, MY_CONTROL_BUF_SIZE);
	if (n < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error reading from fifo '%s' (%d: %s)", port->conf->name, MY_CONTROL(port)->path, errno, strerror(errno));
		goto _ERR_port_get;
	}

	buf[MY_CONTROL_BUF_SIZE] = '\0';
	if (n > 0) {
		if (buf[n - 1] == '\n') {
			buf[n - 1] = '\0';
		}
		my_log(MY_LOG_DEBUG, "core/%s: received '%s' from fifo '%s'", port->conf->name, buf, MY_CONTROL(port)->path);
		my_core_handle_command(port->core, buf);
	}
	return 0;

_ERR_port_put:
_ERR_port_get:
	return -1;
}

static my_port_t *my_control_fifo_create(my_core_t *core, my_port_conf_t *conf)
{
	my_port_t *port;
	char *prop;

	port = my_port_create_priv(MY_CONTROL_SIZE);
	if (!port) {
		goto _MY_ERR_alloc;
	}

	prop = my_prop_lookup(conf->properties, "path");
	if (!prop) {
		my_log(MY_LOG_ERROR, "core/%s: missing 'path' property", port->conf->name);
		goto _MY_ERR_conf;
	}

	MY_CONTROL(port)->path = prop;

	return port;

_MY_ERR_conf:
	my_port_destroy_priv(port);
_MY_ERR_alloc:
	return NULL;
}

static void my_control_fifo_destroy(my_port_t *port)
{
	my_port_destroy_priv(port);
}

static int my_control_fifo_open(my_port_t *port)
{
	MY_DEBUG("core/%s: creating fifo '%s'", port->conf->name, MY_CONTROL(port)->path);
	if (mkfifo(MY_CONTROL(port)->path, 0600) == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error creating fifo '%s' (%d: %s)", port->conf->name, MY_CONTROL(port)->path, errno, strerror(errno));
		goto _MY_ERR_create_fifo;
	}

	MY_DEBUG("core/%s: opening fifo '%s'", port->conf->name, MY_CONTROL(port)->path);
	MY_CONTROL(port)->fd = open(MY_CONTROL(port)->path, O_RDWR | O_NONBLOCK, 0);
	if (MY_CONTROL(port)->fd == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error opening fifo '%s' (%d: %s)", port->conf->name, MY_CONTROL(port)->path, errno, strerror(errno));
		goto _MY_ERR_open_fifo;
	}

	my_core_event_handler_add(port->core, MY_CONTROL(port)->fd, my_control_fifo_event_handler, port);

	return 0;

_MY_ERR_open_fifo:
_MY_ERR_create_fifo:
	return -1;
}

static int my_control_fifo_close(my_port_t *port)
{
	my_core_event_handler_del(port->core, MY_CONTROL(port)->fd);

	MY_DEBUG("core/%s: closing fifo '%s'", port->conf->name, MY_CONTROL(port)->path);
	if (close(MY_CONTROL(port)->fd) == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error closing fifo '%s' (%d: %s)", port->conf->name, MY_CONTROL(port)->path, errno, strerror(errno));
	}

	MY_DEBUG("core/%s: removing fifo '%s'", port->conf->name, MY_CONTROL(port)->path);
	if (unlink(MY_CONTROL(port)->path) == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error removing fifo '%s' (%d: %s)", port->conf->name, MY_CONTROL(port)->path, errno, strerror(errno));
	}

	return 0;
}

my_port_impl_t my_control_fifo = {
	.name = "fifo",
	.desc = "FIFO (named pipe) control interface",
	.create = my_control_fifo_create,
	.destroy = my_control_fifo_destroy,
	.open = my_control_fifo_open,
	.close = my_control_fifo_close,
};
