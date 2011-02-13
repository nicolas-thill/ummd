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
#include "util/url.h"

typedef struct my_control_priv_s my_control_priv_t;

struct my_control_priv_s {
	my_cport_t _inherited;
	char *path;
	int fd;
};

#define MY_CONTROL(p) ((my_control_priv_t *)(p))
#define MY_CONTROL_SIZE (sizeof(my_control_priv_t))


#define MY_CONTROL_BUF_SIZE  255

static void my_control_fifo_event_handler(int fd, void *p)
{
	my_port_t *control = (my_port_t *)p;
	char buf[MY_CONTROL_BUF_SIZE + 1];
	int n;

	n = read(fd, buf, MY_CONTROL_BUF_SIZE);
	if (n == -1) {
		my_log(MY_LOG_ERROR, "core/control: error reading from fifo '%s' (%s)", MY_CONTROL(control)->path, strerror(errno));
	}
	buf[MY_CONTROL_BUF_SIZE] = '\0';
	if (n > 0) {
		if (buf[n - 1] == '\n') {
			buf[n - 1] = '\0';
		}
		my_log(MY_LOG_DEBUG, "core/control: received '%s' from fifo '%s'", buf, MY_CONTROL(control)->path);
		my_core_handle_command(control->core, buf);
	}
}

static my_port_t *my_control_fifo_create(my_core_t *core, my_port_conf_t *conf)
{
	my_port_t *port;
	char *url;
	char url_prot[5];
	char url_path[255];

	port = my_port_create_priv(MY_CONTROL_SIZE);
	if (!port) {
		goto _MY_ERR_alloc;
	}

	url = my_prop_lookup(conf->properties, "url");
	if (!url) {
		my_log(MY_LOG_ERROR, "core/control: missing 'url' property", url);
		goto _MY_ERR_parse_url;
	}

	my_url_split(
		url_prot, sizeof(url_prot),
		NULL, 0, /* auth */
		NULL, 0, /* hostname */
		NULL, /* port */
		url_path, sizeof(url_path),
		url
	);
	if (strlen(url_path) == 0) {
		my_log(MY_LOG_ERROR, "core/control: missing path component in '%s'", url);
		goto _MY_ERR_parse_url;
	}
	MY_CONTROL(port)->path = strdup(url_path);

	return port;

	free(MY_CONTROL(port)->path);
_MY_ERR_parse_url:
	my_port_destroy_priv(port);
_MY_ERR_alloc:
	return NULL;
}

static void my_control_fifo_destroy(my_port_t *port)
{
	free(MY_CONTROL(port)->path);
	my_port_destroy_priv(port);
}

static int my_control_fifo_open(my_port_t *port)
{
	MY_DEBUG("core/control: creating fifo '%s'", MY_CONTROL(port)->path);
	if (mkfifo(MY_CONTROL(port)->path, 0600) == -1) {
		my_log(MY_LOG_ERROR, "core/control: error creating fifo '%s' (%s)", MY_CONTROL(port)->path, strerror(errno));
		goto _MY_ERR_create_fifo;
	}

	MY_DEBUG("core/control: opening fifo '%s'", MY_CONTROL(port)->path);
	MY_CONTROL(port)->fd = open(MY_CONTROL(port)->path, O_RDWR | O_NONBLOCK, 0);
	if (MY_CONTROL(port)->fd == -1) {
		my_log(MY_LOG_ERROR, "core/control: error opening fifo '%s' (%s)", MY_CONTROL(port)->path, strerror(errno));
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

	MY_DEBUG("core/control: closing fifo '%s'", MY_CONTROL(port)->path);
	if (close(MY_CONTROL(port)->fd) == -1) {
		my_log(MY_LOG_ERROR, "core/control: error closing fifo '%s' (%s)", MY_CONTROL(port)->path, strerror(errno));
	}

	MY_DEBUG("core/control: removing fifo '%s'", MY_CONTROL(port)->path);
	if (unlink(MY_CONTROL(port)->path) == -1) {
		my_log(MY_LOG_ERROR, "core/control: error removing fifo '%s' (%s)", MY_CONTROL(port)->path, strerror(errno));
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
