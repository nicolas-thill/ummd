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

typedef struct my_file_priv_s my_file_priv_t;

struct my_file_priv_s {
	my_dport_t _inherited;
	char *path;
	int fd;
};

#define MY_FILE(p) ((my_file_priv_t *)(p))
#define MY_FILE_SIZE (sizeof(my_file_priv_t))

static int my_source_file_event_handler(int fd, void *p)
{
	my_file_priv_t *source = MY_FILE(p);

	/* do something */
	return 0;
}

static int my_target_file_event_handler(int fd, void *p)
{
	my_file_priv_t *source = MY_FILE(p);

	/* do something */
	return 0;
}

static my_port_t *my_io_file_create(my_core_t *core, my_port_conf_t *conf)
{
	my_port_t *port;
	char *prop;

	port = my_port_create_priv(MY_FILE_SIZE);
	if (!port) {
		goto _MY_ERR_create_source;
	}

	prop = my_prop_lookup(conf->properties, "path");
	if (!prop) {
		my_log(MY_LOG_ERROR, "core/%s: missing 'path' property", port->conf->name);
		goto _MY_ERR_conf;
	}
	MY_FILE(port)->path = prop;

	return port;

_MY_ERR_conf:
	my_port_destroy_priv(port);
_MY_ERR_create_source:
	return NULL;
}

static void my_io_file_destroy(my_port_t *port)
{
	my_port_destroy_priv(port);
}

static int my_io_file_open(my_port_t *port)
{
	int rc;

	MY_DEBUG("core/%s: opening file '%s'", port->conf->name, MY_FILE(port)->path);
	MY_FILE(port)->fd = open(MY_FILE(port)->path, O_RDWR, 0);
	if (MY_FILE(port)->fd == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error opening file '%s' (%s)", port->conf->name, MY_FILE(port)->path, strerror(errno));
		goto _MY_ERR_open_file;
	}

	MY_DEBUG("core/%s: putting file in non-blocking mode", port->conf->name);
	rc = my_sock_set_nonblock(MY_FILE(port)->fd);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error putting file in non-blocking mode (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_set_nonblock;
	}

	my_core_event_handler_add(port->core, MY_FILE(port)->fd, MY_PORT_GET_IMPL(port)->handler, port);
	return 0;

_MY_ERR_open_file:
_MY_ERR_set_nonblock:
	return -1;
}

static int my_io_file_close(my_port_t *port)
{
	int ret;

	my_core_event_handler_del(port->core, MY_FILE(port)->fd);
	MY_DEBUG("core/%s: closing file '%s'", port->conf->name, MY_FILE(port)->path);

	ret = close(MY_FILE(port)->fd);
	if (ret < 0)
		my_log(MY_LOG_ERROR, "core/%s: error closing file '%s' (%d: %s)", port->conf->name, MY_FILE(port)->path, errno, strerror(errno));

	return ret;
}

static int my_io_file_get(my_port_t *port, void *buf, int len)
{
	int ret;

	ret = read(MY_FILE(port)->fd, buf, len);
	if (ret < 0) {
		if ((errno == EWOULDBLOCK) || (errno == EINTR)) {
			ret = 0;
			goto out;
		}

		my_log(MY_LOG_ERROR, "core/%s: error reading from file '%s' (%d: %s)", port->conf->name, MY_FILE(port)->path, errno, strerror(errno));
		return ret;
	}

out:
	MY_DEBUG("core/%s: read %d bytes from file '%s'", port->conf->name, ret, MY_FILE(port)->path);
	return ret;
}

static int my_io_file_put(my_port_t *port, void *buf, int len)
{
	int ret;

	ret = write(MY_FILE(port)->fd, buf, len);
	if (ret < 0) {
		if ((errno == EWOULDBLOCK) || (errno == EINTR)) {
			ret = 0;
			goto out;
		}

		my_log(MY_LOG_ERROR, "core/%s: error writing to file '%s' (%d: %s)", port->conf->name, MY_FILE(port)->path, errno, strerror(errno));
		goto out;
	}

	MY_DEBUG("core/%s: wrote %d bytes to file '%s'", port->conf->name, ret, MY_FILE(port)->path);

out:
	return ret;
}

my_port_impl_t my_source_file = {
	.name = "file",
	.desc = "Regular file source",
	.create = my_io_file_create,
	.destroy = my_io_file_destroy,
	.open = my_io_file_open,
	.close = my_io_file_close,
	.get = my_io_file_get,
	.handler = my_source_file_event_handler,
};

my_port_impl_t my_target_file = {
	.name = "file",
	.desc = "Regular file target",
	.create = my_io_file_create,
	.destroy = my_io_file_destroy,
	.open = my_io_file_open,
	.close = my_io_file_close,
	.put = my_io_file_put,
	.handler = my_target_file_event_handler,
};
