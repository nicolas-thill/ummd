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

typedef struct my_source_priv_s my_source_priv_t;

struct my_source_priv_s {
	my_dport_t _inherited;
	char *path;
	int fd;
};

#define MY_SOURCE(p) ((my_source_priv_t *)(p))
#define MY_SOURCE_SIZE (sizeof(my_source_priv_t))

static int my_source_file_event_handler(int fd, void *p)
{
	my_port_t *port = MY_PORT(p), *peer = MY_PORT(MY_DPORT(port)->peer);
	char buf[1024];
	int n = sizeof(buf);

	n = my_port_get(port, buf, n);
	if (n == -1) {
		goto _ERR_port_get;
	}

	n = my_port_put(peer, buf, n);
	if (n == -1) {
		goto _ERR_port_put;
	}

	return 0;

_ERR_port_put:
_ERR_port_get:
	return -1;
}

static my_port_t *my_source_file_create(my_port_conf_t *conf)
{
	my_port_t *port;
	char *url;
	char url_prot[5];
	char url_path[255];

	port = my_port_priv_create(conf, MY_SOURCE_SIZE);
	if (!port) {
		goto _MY_ERR_create_source;
	}

	url = my_prop_lookup(conf->properties, "url");
	if (!url) {
		my_log(MY_LOG_ERROR, "core/source: missing 'url' property");
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
		my_log(MY_LOG_ERROR, "core/source: missing path component in '%s'", url);
		goto _MY_ERR_parse_url;
	}
	MY_SOURCE(port)->path = strdup(url_path);

	return port;

	free(MY_SOURCE(port)->path);
_MY_ERR_parse_url:
	my_port_priv_destroy(port);
_MY_ERR_create_source:
	return NULL;
}

static void my_source_file_destroy(my_port_t *port)
{
	free(MY_SOURCE(port)->path);
	my_port_priv_destroy(port);
}

static int my_source_file_open(my_port_t *port)
{
	MY_DEBUG("core/source: opening file '%s'", MY_SOURCE(port)->path);
	MY_SOURCE(port)->fd = open(MY_SOURCE(port)->path, O_RDONLY, 0);
	if (MY_SOURCE(port)->fd == -1) {
		my_log(MY_LOG_ERROR, "core/source: error opening file '%s' (%s)", MY_SOURCE(port)->path, strerror(errno));
		goto _MY_ERR_open_file;
	}

	my_core_event_handler_add(port->core, MY_SOURCE(port)->fd, my_source_file_event_handler, port);

	return 0;

_MY_ERR_open_file:
	return -1;
}

static int my_source_file_close(my_port_t *port)
{
	my_core_event_handler_del(port->core, MY_SOURCE(port)->fd);

	MY_DEBUG("core/source: closing file '%s'", MY_SOURCE(port)->path);
	if (close(MY_SOURCE(port)->fd) == -1) {
		my_log(MY_LOG_ERROR, "core/source: error closing file '%s' (%s)", MY_SOURCE(port)->path, strerror(errno));
	}

	return 0;
}

static int my_source_file_get(my_port_t *port, void *buf, int len)
{
	int n;

	n = read(MY_SOURCE(port)->fd, buf, len);
	if (n == -1) {
		my_log(MY_LOG_ERROR, "core/source: error reading from file '%s' (%s)", MY_SOURCE(port)->path, strerror(errno));
		return -1;
	}

	MY_DEBUG("core/source: read %d bytes from '%s'", n, MY_SOURCE(port)->path);

	return n;
}

my_port_impl_t my_source_file = {
	.name = "file",
	.desc = "Regular file source",
	.create = my_source_file_create,
	.destroy = my_source_file_destroy,
	.open = my_source_file_open,
	.close = my_source_file_close,
	.get = my_source_file_get,
};
