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

#include "core/controls.h"

#include "util/log.h"
#include "util/mem.h"

typedef struct my_control_priv my_control_priv_t;

struct my_control_priv {
	my_control_t base;
	char *path;
	int sock;
};

#define MY_CONTROL_PRIV(p) ((my_control_priv_t *)(p))

static my_control_t *my_control_sock_create(my_control_conf_t *conf)
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
		if ((strncmp(conf->url, "file", n) != 0) && (strncmp(conf->url, "sock", n) != 0)) {
			my_log(MY_LOG_ERROR, "core/control/sock: unknown method '%.2$*1$s' in url '%$1s'", conf->url, n);
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

static void my_control_sock_destroy(my_control_t *control)
{
	my_mem_free(control);
}

static int my_control_sock_open(my_control_t *control)
{
	struct sockaddr_un sa;

	MY_DEBUG("core/control/sock: creating unix socket '%s'", MY_CONTROL_PRIV(control)->path);
	MY_CONTROL_PRIV(control)->sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (MY_CONTROL_PRIV(control)->sock == -1) {
		my_log(MY_LOG_ERROR, "core/control/sock: error creating socket '%s' (%s)", MY_CONTROL_PRIV(control)->path, strerror(errno));
		goto _MY_ERR_create_sock;
	}

	my_mem_zero(&sa, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, MY_CONTROL_PRIV(control)->path, sizeof(sa.sun_path));
	
	MY_DEBUG("core/control/sock: binding unix socket '%s'", MY_CONTROL_PRIV(control)->path);
	if (bind(MY_CONTROL_PRIV(control)->sock, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
		my_log(MY_LOG_ERROR, "core/control/sock: error binding unix socket '%s' (%s)", MY_CONTROL_PRIV(control)->path, strerror(errno));
		goto _MY_ERR_bind_sock;
	}

	return 0;

_MY_ERR_bind_sock:
	close(MY_CONTROL_PRIV(control)->sock);
_MY_ERR_create_sock:
	return -1;
}

static int my_control_sock_close(my_control_t *control)
{
	MY_DEBUG("core/control/sock: closing unix socket '%s'", MY_CONTROL_PRIV(control)->path);
	if (close(MY_CONTROL_PRIV(control)->sock) == -1) {
		my_log(MY_LOG_ERROR, "core/control/sock: error closing unix socket '%s' (%s)", MY_CONTROL_PRIV(control)->path, strerror(errno));
		return -1;
	}

	MY_DEBUG("core/control/fifo: removing unix socket '%s'", MY_CONTROL_PRIV(control)->path);
	if (unlink(MY_CONTROL_PRIV(control)->path) == -1) {
		my_log(MY_LOG_ERROR, "core/control/fifo: error removing unix socket '%s' (%s)", MY_CONTROL_PRIV(control)->path, strerror(errno));
		return -1;
	}

	return 0;
}

my_control_impl_t my_control_sock = {
	.id = MY_CONTROL_SOCK,
	.name = "sock",
	.desc = "Unix socket control interface",
	.create = my_control_sock_create,
	.destroy = my_control_sock_destroy,
	.open = my_control_sock_open,
	.close = my_control_sock_close,
};