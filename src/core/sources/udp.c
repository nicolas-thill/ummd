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

#include <arpa/inet.h>

typedef struct my_source_data_s my_source_data_t;

struct my_source_data_s {
	my_dport_t _inherited;
	char *path;
	unsigned int ip_addr;
	unsigned short port;
	int fd;
};

#define MY_SOURCE(p) ((my_source_data_t *)(p))
#define MY_SOURCE_SIZE (sizeof(my_source_data_t))

static int my_source_udp_event_handler(int fd, void *p)
{
	my_port_t *port = MY_PORT(p), *peer = MY_PORT(MY_DPORT(port)->peer);
	char buf[1024];
	int n = sizeof(buf);

	n = my_port_get(port, buf, n);
	if (n < 0) {
		goto _ERR_port_get;
	}

	n = my_port_put(peer, buf, n);
	if (n < 0) {
		goto _ERR_port_put;
	}

	return 0;

_ERR_port_put:
_ERR_port_get:
	return -1;
}

static my_port_t *my_source_udp_create(my_core_t *core, my_port_conf_t *conf)
{
	struct in_addr ip_addr_tmp;
	my_port_t *port;
	char *url;
	char url_prot[5], url_ip[20], url_path[255];
	int url_port, ret;

	port = my_port_create_priv(MY_SOURCE_SIZE);
	if (!port) {
		goto _MY_ERR_create_source;
	}

	url = my_prop_lookup(conf->properties, "url");
	if (url) {
		my_url_split(
			url_prot, sizeof(url_prot),
			NULL, 0, /* auth */
			url_ip, sizeof(url_ip),
			url_port,
			NULL, 0, /* url path */
			url
		);

		if (strlen(url_prot) > 0) {
			if (strcmp(url_prot, "udp") != 0) {
				my_log(MY_LOG_ERROR, "core/source: unknown url protocol '%s' in '%s'", url_prot, url);
				goto _MY_ERR_parse_url;
			}
		}

		if (strlen(url_ip) == 0) {
			my_log(MY_LOG_ERROR, "core/source: missing ip component in '%s'", url);
			goto _MY_ERR_parse_url;
		}

		ret = inet_pton(AF_INET, url_ip, &ip_addr_tmp);
		if (ret != 1) {
			my_log(MY_LOG_ERROR, "core/source: malformed ip component in '%s'", url);
			goto _MY_ERR_parse_url;
		}

		if (url_port == 0) {
			my_log(MY_LOG_ERROR, "core/source: missing port component in '%s'", url);
			goto _MY_ERR_parse_url;
		}

		MY_SOURCE(port)->path = strdup(url_path);
		MY_SOURCE(port)->ip_addr = ip_addr_tmp.s_addr;
		MY_SOURCE(port)->port = (unsigned short)url_port;
	}

	return port;

	free(MY_SOURCE(port)->path);
_MY_ERR_parse_url:
	my_port_priv_destroy(port);
_MY_ERR_create_source:
	return NULL;
}

static void my_source_udp_destroy(my_port_t *port)
{
	free(MY_SOURCE(port)->path);
	my_port_destroy_priv(port);
}

static int my_source_udp_open(my_port_t *port)
{
	int sock_opts;

	MY_DEBUG("core/source: opening file '%s'", MY_SOURCE(port)->path);
	MY_SOURCE(port)->fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (MY_SOURCE(port)->fd < 0) {
		my_log(MY_LOG_ERROR, "core/source: error opening file '%s' (%s)", MY_SOURCE(port)->path, strerror(errno));
		goto _MY_ERR_open_file;
	}

	sock_opts = fcntl(MY_SOURCE(port)->fd, F_GETFL, 0);
	fcntl(MY_SOURCE(port)->fd, F_SETFL, sock_opts | O_NONBLOCK);

	my_core_event_handler_add(port->core, MY_SOURCE(port)->fd, my_source_udp_event_handler, port);

	return 0;

_MY_ERR_open_file:
	return -1;
}

static int my_source_udp_close(my_port_t *port)
{
	my_core_event_handler_del(port->core, MY_SOURCE(port)->fd);

	MY_DEBUG("core/source: closing file '%s'", MY_SOURCE(port)->path);
	if (close(MY_SOURCE(port)->fd) == -1) {
		my_log(MY_LOG_ERROR, "core/source: error closing file '%s' (%s)", MY_SOURCE(port)->path, strerror(errno));
	}

	return 0;
}

static int my_source_udp_get(my_port_t *port, void *buf, int len)
{
	int n;

	n = read(MY_SOURCE(port)->fd, buf, len);
	if (n < 0) {
		if ((errno == EWOULDBLOCK) || (errno == EINTR)) {
			n = 0;
			goto out;
		}

		my_log(MY_LOG_ERROR, "core/source: error reading from udp source '%s' (%s)", MY_SOURCE(port)->path, strerror(errno));
		return n;
	}

out:
	MY_DEBUG("core/source: read %d bytes from '%s'", n, MY_SOURCE(port)->path);
	return n;
}

my_port_impl_t my_source_udp = {
	.name = "udp",
	.desc = "Multicast source",
	.create = my_source_udp_create,
	.destroy = my_source_udp_destroy,
	.open = my_source_udp_open,
	.close = my_source_udp_close,
	.get = my_source_udp_get,
};
