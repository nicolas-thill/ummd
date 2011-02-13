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
#include "util/url.h"

typedef struct my_control_priv_s my_control_priv_t;

struct my_control_priv_s {
	my_cport_t _inherited;
	char *path;
	int sock;
};

#define MY_CONTROL(p) ((my_control_priv_t *)(p))
#define MY_CONTROL_SIZE (sizeof(my_control_priv_t))

static my_port_t *my_control_sock_create(my_port_conf_t *conf)
{
	my_port_t *port;
	char *url;
	char url_prot[5];
	char url_path[255];

	port = my_port_priv_create(conf, MY_CONTROL_SIZE);
	if (!port) {
		goto _MY_ERR_alloc;
	}

	url = my_prop_lookup(conf->properties, "url");
	if (!url) {
		my_log(MY_LOG_ERROR, "core/control: missing 'url' property");
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
	my_port_priv_destroy(port);
_MY_ERR_alloc:
	return NULL;
}

static void my_control_sock_destroy(my_port_t *port)
{
	free(MY_CONTROL(port)->path);
	my_port_priv_destroy(port);
}

static int my_control_sock_open(my_port_t *port)
{
	struct sockaddr_un sa;

	MY_DEBUG("core/control: creating unix socket '%s'", MY_CONTROL(port)->path);
	MY_CONTROL(port)->sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (MY_CONTROL(port)->sock == -1) {
		my_log(MY_LOG_ERROR, "core/control: error creating socket '%s' (%s)", MY_CONTROL(port)->path, strerror(errno));
		goto _MY_ERR_create_sock;
	}

	my_mem_zero(&sa, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, MY_CONTROL(port)->path, sizeof(sa.sun_path));
	
	MY_DEBUG("core/control: binding unix socket '%s'", MY_CONTROL(port)->path);
	if (bind(MY_CONTROL(port)->sock, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
		my_log(MY_LOG_ERROR, "core/control: error binding unix socket '%s' (%s)", MY_CONTROL(port)->path, strerror(errno));
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
	MY_DEBUG("core/control: closing unix socket '%s'", MY_CONTROL(port)->path);
	if (close(MY_CONTROL(port)->sock) == -1) {
		my_log(MY_LOG_ERROR, "core/control: error closing unix socket '%s' (%s)", MY_CONTROL(port)->path, strerror(errno));
	}

	MY_DEBUG("core/control: removing unix socket '%s'", MY_CONTROL(port)->path);
	if (unlink(MY_CONTROL(port)->path) == -1) {
		my_log(MY_LOG_ERROR, "core/control: error removing unix socket '%s' (%s)", MY_CONTROL(port)->path, strerror(errno));
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
