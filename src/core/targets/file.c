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

typedef struct my_target_priv_s my_target_priv_t;

struct my_target_priv_s {
	my_dport_t _inherited;
	char *path;
	int fd;
};

#define MY_TARGET(p) ((my_target_priv_t *)(p))
#define MY_TARGET_SIZE (sizeof(my_target_priv_t))

static int my_target_file_event_handler(int fd, void *p)
{
	my_target_priv_t *source = MY_TARGET(p);

	/* do something */
	return 0;
}

static my_port_t *my_target_file_create(my_core_t *core, my_port_conf_t *conf)
{
	my_port_t *port;
	char *prop;

	port = my_port_create_priv(MY_TARGET_SIZE);
	if (!port) {
		goto _MY_ERR_create_source;
	}

	prop = my_prop_lookup(conf->properties, "path");
	if (!prop) {
		my_log(MY_LOG_ERROR, "core/%s: missing 'path' property", port->conf->name);
		goto _MY_ERR_conf;
	}
	MY_TARGET(port)->path = prop;

	return port;

_MY_ERR_conf:
	my_port_destroy_priv(port);
_MY_ERR_create_source:
	return NULL;
}

static void my_target_file_destroy(my_port_t *port)
{
	my_port_destroy_priv(port);
}

static int my_target_file_open(my_port_t *port)
{
	MY_DEBUG("core/%s: opening file '%s'", port->conf->name, MY_TARGET(port)->path);
	MY_TARGET(port)->fd = open(MY_TARGET(port)->path, O_WRONLY, 0);
	if (MY_TARGET(port)->fd == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error opening file '%s' (%s)", port->conf->name, MY_TARGET(port)->path, strerror(errno));
		goto _MY_ERR_open_file;
	}

	my_core_event_handler_add(port->core, MY_TARGET(port)->fd, my_target_file_event_handler, port);

	return 0;

_MY_ERR_open_file:
	return -1;
}

static int my_target_file_close(my_port_t *port)
{
	my_core_event_handler_del(port->core, MY_TARGET(port)->fd);

	MY_DEBUG("core/%s: closing file '%s'", port->conf->name, MY_TARGET(port)->path);
	if (close(MY_TARGET(port)->fd) == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error closing file '%s' (%d: %s)", port->conf->name, MY_TARGET(port)->path, errno, strerror(errno));
	}

	return 0;
}

static int my_target_file_put(my_port_t *port, void *buf, int len)
{
	int n;

	n = write(MY_TARGET(port)->fd, buf, len);
	if (n == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error writing to file '%s' (%d: %s)", port->conf->name, MY_TARGET(port)->path, errno, strerror(errno));
		return -1;
	}

	MY_DEBUG("core/%s: wrote %d bytes to file '%s'", port->conf->name, n, MY_TARGET(port)->path);

	return n;
}

my_port_impl_t my_target_file = {
	.name = "file",
	.desc = "Regular file target",
	.create = my_target_file_create,
	.destroy = my_target_file_destroy,
	.open = my_target_file_open,
	.close = my_target_file_close,
	.put = my_target_file_put,
};
