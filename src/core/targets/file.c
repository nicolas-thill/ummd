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
	char *url;
	char url_prot[5];
	char url_path[255];

	port = my_port_create_priv(MY_TARGET_SIZE);
	if (!port) {
		goto _MY_ERR_create_source;
	}

	url = my_prop_lookup(conf->properties, "url");
	if (!url) {
		my_log(MY_LOG_ERROR, "core/target: missing 'url' property");
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
		my_log(MY_LOG_ERROR, "core/target: missing path component in '%s'", url);
		goto _MY_ERR_parse_url;
	}
	MY_TARGET(port)->path = strdup(url_path);

	return port;

	free(MY_TARGET(port)->path);
_MY_ERR_parse_url:
	my_port_destroy_priv(port);
_MY_ERR_create_source:
	return NULL;
}

static void my_target_file_destroy(my_port_t *port)
{
	free(MY_TARGET(port)->path);
	my_port_destroy_priv(port);
}

static int my_target_file_open(my_port_t *port)
{
	MY_DEBUG("core/target: opening file '%s'", MY_TARGET(port)->path);
	MY_TARGET(port)->fd = open(MY_TARGET(port)->path, O_WRONLY, 0);
	if (MY_TARGET(port)->fd == -1) {
		my_log(MY_LOG_ERROR, "core/target: error opening file '%s' (%s)", MY_TARGET(port)->path, strerror(errno));
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

	MY_DEBUG("core/target: closing file '%s'", MY_TARGET(port)->path);
	if (close(MY_TARGET(port)->fd) == -1) {
		my_log(MY_LOG_ERROR, "core/target: error closing file '%s' (%s)", MY_TARGET(port)->path, strerror(errno));
	}

	return 0;
}

static int my_target_file_put(my_port_t *port, void *buf, int len)
{
	int n;

	n = write(MY_TARGET(port)->fd, buf, len);
	if (n == -1) {
		my_log(MY_LOG_ERROR, "core/target: error writing from file '%s' (%d: %s)", MY_TARGET(port)->path, errno, strerror(errno));
		return -1;
	}

	MY_DEBUG("core/source: wrote %d bytes to '%s'", n, MY_TARGET(port)->path);

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
