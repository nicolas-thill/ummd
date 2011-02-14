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
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "core/ports.h"

#include "util/log.h"
#include "util/mem.h"
#include "util/prop.h"

typedef struct my_control_priv_s my_control_priv_t;

struct my_control_priv_s {
	my_cport_t _inherited;
	char *path;
	int sock;
};

#define MY_CONTROL(p) ((my_control_priv_t *)(p))
#define MY_CONTROL_SIZE (sizeof(my_control_priv_t))

static my_port_t *my_control_sock_create(my_core_t *core, my_port_conf_t *conf)
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

static void my_control_sock_destroy(my_port_t *port)
{
	my_port_destroy_priv(port);
}

static int my_control_sock_open(my_port_t *port)
{
	struct sockaddr_un sa;

	MY_DEBUG("core/%s: creating unix socket '%s'", port->conf->name, MY_CONTROL(port)->path);
	MY_CONTROL(port)->sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (MY_CONTROL(port)->sock == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error creating socket '%s' (%d: %s)", port->conf->name, MY_CONTROL(port)->path, errno, strerror(errno));
		goto _MY_ERR_create_sock;
	}

	my_mem_zero(&sa, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, MY_CONTROL(port)->path, sizeof(sa.sun_path));

	MY_DEBUG("core/%s: binding unix socket '%s'", port->conf->name, MY_CONTROL(port)->path);
	if (bind(MY_CONTROL(port)->sock, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error binding unix socket '%s' (%d: %s)", port->conf->name, MY_CONTROL(port)->path, errno, strerror(errno));
		goto _MY_ERR_bind_sock;
	}

	return 0;

_MY_ERR_bind_sock:
	close(MY_CONTROL(port)->sock);
_MY_ERR_create_sock:
	return -1;
}

static int my_control_sock_close(my_port_t *port)
{
	MY_DEBUG("core/%s: closing unix socket '%s'", port->conf->name, MY_CONTROL(port)->path);
	if (close(MY_CONTROL(port)->sock) == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error closing unix socket '%s' (%d: %s)", port->conf->name, MY_CONTROL(port)->path, errno, strerror(errno));
	}

	MY_DEBUG("core/%s: removing unix socket '%s'", port->conf->name, MY_CONTROL(port)->path);
	if (unlink(MY_CONTROL(port)->path) == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error removing unix socket '%s' (%d: %s)", port->conf->name, MY_CONTROL(port)->path, errno, strerror(errno));
	}

	return 0;
}

my_port_impl_t my_control_sock = {
	.name = "sock",
	.desc = "Unix socket control interface",
	.create = my_control_sock_create,
	.destroy = my_control_sock_destroy,
	.open = my_control_sock_open,
	.close = my_control_sock_close,
};
