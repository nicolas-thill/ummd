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

#include "util/audio.h"
#include "util/log.h"
#include "util/mem.h"
#include "util/prop.h"

typedef struct my_source_priv_s my_source_priv_t;

struct my_source_priv_s {
	my_dport_t _inherited;
	my_audio_codec_t *codec;
	char *path;
	int fd;
};

#define MY_SOURCE(p) ((my_source_priv_t *)(p))
#define MY_SOURCE_SIZE (sizeof(my_source_priv_t))

static int my_source_file_event_handler(int fd, void *p)
{
	my_port_t *port = MY_PORT(p), *peer = MY_PORT(MY_DPORT(port)->peer);
	u_int8_t ibuf[16384], obuf[196608];
	int ilen, olen;
	u_int8_t *iptr;
	int i, n;

	ilen = sizeof(ibuf);
	ilen = my_port_get(port, ibuf, ilen);
	if (ilen < -1) {
		goto _ERR_port_get;
	}

	iptr = ibuf;
	olen = sizeof(obuf);
	while (ilen > 0) {
		i = ilen;
		n = my_audio_decode(MY_SOURCE(port)->codec, iptr, &i, obuf, &olen);
		if (n <= 0) {
			break;
		}
		if (olen > 0) {
			olen = my_port_put(peer, obuf, olen);
			if (olen < -1) {
				goto _ERR_port_put;
			}
		}
		ilen -= i;
		iptr += i;
	}

	return 0;

_ERR_port_put:
_ERR_port_get:
	return -1;
}

static my_port_t *my_source_file_create(my_core_t *core, my_port_conf_t *conf)
{
	my_port_t *port;
	char *prop;
	char url_prot[5];
	char url_path[255];

	port = my_port_create_priv(MY_SOURCE_SIZE);
	if (!port) {
		goto _MY_ERR_create_source;
	}

	prop = my_prop_lookup(conf->properties, "path");
	if (!prop) {
		my_log(MY_LOG_ERROR, "core/%s: missing 'path' property");
		goto _MY_ERR_conf;
	}
	MY_SOURCE(port)->path = prop;

	prop = my_prop_lookup(conf->properties, "audio-format");
	MY_SOURCE(port)->codec = my_audio_codec_create(prop);

	return port;

_MY_ERR_conf:
	my_port_destroy_priv(port);
_MY_ERR_create_source:
	return NULL;
}

static void my_source_file_destroy(my_port_t *port)
{
	my_audio_codec_destroy(MY_SOURCE(port)->codec);
	my_port_destroy_priv(port);
}

static int my_source_file_open(my_port_t *port)
{
	int sock_opts;

	MY_DEBUG("core/%s: opening file '%s'", port->conf->name, MY_SOURCE(port)->path);
	MY_SOURCE(port)->fd = open(MY_SOURCE(port)->path, O_RDONLY, 0);
	if (MY_SOURCE(port)->fd == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error opening file '%s' (%d: %s)", port->conf->name, MY_SOURCE(port)->path, errno, strerror(errno));
		goto _MY_ERR_open_file;
	}

	sock_opts = fcntl(MY_SOURCE(port)->fd, F_GETFL, 0);
	fcntl(MY_SOURCE(port)->fd, F_SETFL, sock_opts | O_NONBLOCK);

	my_core_event_handler_add(port->core, MY_SOURCE(port)->fd, my_source_file_event_handler, port);

	return 0;

_MY_ERR_open_file:
	return -1;
}

static int my_source_file_close(my_port_t *port)
{
	my_core_event_handler_del(port->core, MY_SOURCE(port)->fd);

	MY_DEBUG("core/%s: closing file '%s'", port->conf->name, MY_SOURCE(port)->path);
	if (close(MY_SOURCE(port)->fd) == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error closing file '%s' (%d: %s)", port->conf->name, MY_SOURCE(port)->path, errno, strerror(errno));
	}

	return 0;
}

static int my_source_file_get(my_port_t *port, void *buf, int len)
{
	int n;

	n = read(MY_SOURCE(port)->fd, buf, len);
	if (n < 0) {
		if ((errno == EWOULDBLOCK) || (errno == EINTR)) {
			n = 0;
			goto out;
		}

		my_log(MY_LOG_ERROR, "core/%s: error reading from file '%s' (%d: %s)", port->conf->name, MY_SOURCE(port)->path, errno, strerror(errno));
		return n;
	}

out:
	MY_DEBUG("core/%s: read %d bytes from file '%s'", port->conf->name, n, MY_SOURCE(port)->path);
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
