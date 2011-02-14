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

#include <arpa/inet.h>
#include <netinet/in.h>

typedef struct my_source_data_s my_source_data_t;

struct my_source_data_s {
	my_dport_t _inherited;
	char *ip_addr;
	int ip_port;
	int fd;
	my_audio_codec_t *codec;
};

#define MY_SOURCE(p) ((my_source_data_t *)(p))
#define MY_SOURCE_SIZE (sizeof(my_source_data_t))

static int my_source_udp_event_handler(int fd, void *p)
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
		n = my_audio_codec_decode(MY_SOURCE(port)->codec, iptr, &i, obuf, &olen);
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

static my_port_t *my_source_udp_create(my_core_t *core, my_port_conf_t *conf)
{
	my_port_t *port;
	char *prop;
	struct in_addr ia;
	int ret;

	port = my_port_create_priv(MY_SOURCE_SIZE);
	if (!port) {
		goto _MY_ERR_create_source;
	}

	prop = my_prop_lookup(conf->properties, "host");
	if (!prop) {
		my_log(MY_LOG_ERROR, "core/%s: missing 'host' property", port->conf->name);
		goto _MY_ERR_conf;
	}
	ret = inet_pton(AF_INET, prop, &ia);
	if (ret != 1) {
		my_log(MY_LOG_ERROR, "core/%s: malformed 'host' property '%s'", port->conf->name, prop);
		goto _MY_ERR_conf;
	}
	MY_SOURCE(port)->ip_addr = prop;

	prop = my_prop_lookup(conf->properties, "port");
	if (!prop) {
		my_log(MY_LOG_ERROR, "core/%s: missing 'port' property", port->conf->name);
		goto _MY_ERR_conf;
	}
	MY_SOURCE(port)->ip_port = atoi(prop);

	prop = my_prop_lookup(conf->properties, "audio-format");
	MY_SOURCE(port)->codec = my_audio_codec_create(prop);

	return port;

_MY_ERR_conf:
	my_port_destroy_priv(port);
_MY_ERR_create_source:
	return NULL;
}

static void my_source_udp_destroy(my_port_t *port)
{
	my_audio_codec_destroy(MY_SOURCE(port)->codec);
	my_port_destroy_priv(port);
}

static int my_source_udp_open(my_port_t *port)
{
	struct sockaddr_in sa;
	struct ip_mreq mr;
	int so;

	MY_DEBUG("core/%s: opening socket", port->conf->name);
	MY_SOURCE(port)->fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (MY_SOURCE(port)->fd < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error opening socket' (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_socket;
	}

	my_mem_zero(&sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(MY_SOURCE(port)->ip_port);
	MY_DEBUG("core/%s: binding socket", port->conf->name);
	if (bind(MY_SOURCE(port)->fd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error binding socket socket (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_bind;
	}

	mr.imr_multiaddr.s_addr = inet_addr(MY_SOURCE(port)->ip_addr);
	mr.imr_interface.s_addr = htonl(INADDR_ANY);
	MY_DEBUG("core/%s: joining mcast group", port->conf->name);
	so = setsockopt(MY_SOURCE(port)->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mr, sizeof(mr));
	if (so == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error joining mcast group (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_setsockopt;
	}

	so = 65535;
	MY_DEBUG("core/%s: setting socket receive buffer to %d", port->conf->name, so);
	so = setsockopt(MY_SOURCE(port)->fd, SOL_SOCKET, SO_RCVBUF, &so, sizeof(so));
	if (so == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error setting socket receive buffer (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_setsockopt;
	}

	MY_DEBUG("core/%s: setting socket to non-blocking mode", port->conf->name);
	so = fcntl(MY_SOURCE(port)->fd, F_GETFL, 0);
	if (so == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error getting socket flags (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_fcntl;
	}
	so = fcntl(MY_SOURCE(port)->fd, F_SETFL, so | O_NONBLOCK);
	if (so == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error setting socket flags (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_fcntl;
	}

	my_core_event_handler_add(port->core, MY_SOURCE(port)->fd, my_source_udp_event_handler, port);

	return 0;

_MY_ERR_setsockopt:
_MY_ERR_bind:
_MY_ERR_fcntl:
_MY_ERR_socket:
	return -1;
}

static int my_source_udp_close(my_port_t *port)
{
	struct ip_mreq mr;
	int so;

	my_core_event_handler_del(port->core, MY_SOURCE(port)->fd);

	MY_DEBUG("core/%s: leaving mcast group", port->conf->name);
	mr.imr_multiaddr.s_addr = inet_addr(MY_SOURCE(port)->ip_addr);
	mr.imr_interface.s_addr = htonl(INADDR_ANY);
	so = setsockopt(MY_SOURCE(port)->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mr, sizeof(mr));
	if (so == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error leaving mcast group (%d :%s)", port->conf->name, errno, strerror(errno));
	}

	MY_DEBUG("core/%s: closing socket", port->conf->name);
	if (close(MY_SOURCE(port)->fd) == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error closing file (%d: %s)", port->conf->name, errno, strerror(errno));
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

		my_log(MY_LOG_ERROR, "core/%s: error reading from socket (%d: %s)", port->conf->name, errno, strerror(errno));
		return n;
	}

out:
	MY_DEBUG("core/%s: read %d bytes from socket", port->conf->name, n);
	return n;
}

my_port_impl_t my_source_udp = {
	.name = "udp",
	.desc = "UDP multicast source",
	.create = my_source_udp_create,
	.destroy = my_source_udp_destroy,
	.open = my_source_udp_open,
	.close = my_source_udp_close,
	.get = my_source_udp_get,
};
