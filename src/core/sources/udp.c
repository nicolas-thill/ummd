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

typedef struct my_source_priv_s my_source_priv_t;

struct my_source_priv_s {
	my_dport_t _inherited;
	char *ip_addr;
	int ip_port;
	int fd;
	struct sockaddr *sa_local;
	struct sockaddr *sa_group;
	my_audio_codec_t *codec;
};

#define MY_SOURCE(p) ((my_source_priv_t *)(p))
#define MY_SOURCE_SIZE (sizeof(my_source_priv_t))

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
		goto _MY_ERR_port_get;
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
				goto _MY_ERR_port_put;
			}
		}
		ilen -= i;
		iptr += i;
	}

	return 0;

_MY_ERR_port_put:
_MY_ERR_port_get:
	return -1;
}

static my_port_t *my_source_udp_create(my_core_t *core, my_port_conf_t *conf)
{
	my_port_t *port;
	char *prop;
	struct in_addr ia;
	int sa_len;
	int ret;

	port = my_port_create_priv(MY_SOURCE_SIZE);
	if (!port) {
		goto _MY_ERR_create_priv;
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

	sa_len = sizeof(struct sockaddr_in);

	MY_SOURCE(port)->sa_local = my_mem_alloc(sa_len);
	if (!MY_SOURCE(port)->sa_local) {
		goto _MY_ERR_alloc_sa_local;
	}
	my_mem_zero(MY_SOURCE(port)->sa_local, sa_len);

	MY_SOURCE(port)->sa_group = my_mem_alloc(sa_len);
	if (!MY_SOURCE(port)->sa_group) {
		goto _MY_ERR_alloc_sa_group;
	}
	my_mem_zero(MY_SOURCE(port)->sa_group, sa_len);

	((struct sockaddr_in *)MY_SOURCE(port)->sa_local)->sin_family = AF_INET;
	((struct sockaddr_in *)MY_SOURCE(port)->sa_local)->sin_addr.s_addr = htonl(INADDR_ANY);
	((struct sockaddr_in *)MY_SOURCE(port)->sa_local)->sin_port = htons(MY_SOURCE(port)->ip_port);

	((struct sockaddr_in *)MY_SOURCE(port)->sa_group)->sin_family = AF_INET;
	((struct sockaddr_in *)MY_SOURCE(port)->sa_group)->sin_addr.s_addr = inet_addr(MY_SOURCE(port)->ip_addr);

	return port;

_MY_ERR_conf:
	my_mem_free(MY_SOURCE(port)->sa_group);
_MY_ERR_alloc_sa_group:
	my_mem_free(MY_SOURCE(port)->sa_local);
_MY_ERR_alloc_sa_local:
	my_port_destroy_priv(port);
_MY_ERR_create_priv:
	return NULL;
}

static void my_source_udp_destroy(my_port_t *port)
{
	my_mem_free(MY_SOURCE(port)->sa_group);
	my_mem_free(MY_SOURCE(port)->sa_local);
	my_audio_codec_destroy(MY_SOURCE(port)->codec);
	my_port_destroy_priv(port);
}

static int my_source_udp_open(my_port_t *port)
{
	int rc;

	MY_DEBUG("core/%s: opening socket", port->conf->name);
	MY_SOURCE(port)->fd = my_sock_create(PF_INET, SOCK_DGRAM);
	if (MY_SOURCE(port)->fd < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error opening socket' (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_sock_create;
	}

	MY_DEBUG("core/%s: binding socket", port->conf->name);
	rc = my_sock_bind(MY_SOURCE(port)->fd, MY_SOURCE(port)->sa_local);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error binding socket socket (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_sock_bind;
	}

	MY_DEBUG("core/%s: joining mcast group", port->conf->name);
	rc = my_net_mcast_join(MY_SOURCE(port)->fd, MY_SOURCE(port)->sa_local, MY_SOURCE(port)->sa_group);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error joining mcast group (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_mcast_join;
	}

	MY_DEBUG("core/%s: setting socket receive buffer to %d", port->conf->name, 65535);
	rc = my_sock_set_recv_buffer_size(MY_SOURCE(port)->fd, 65535);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error setting socket receive buffer (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_set_recv_buffer_size;
	}

	MY_DEBUG("core/%s: putting socket in non-blocking mode", port->conf->name);
	rc = my_sock_set_nonblock(MY_SOURCE(port)->fd);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error putting socket in non-blocking mode (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_set_nonblock;
	}

	MY_DEBUG("core/%s: reusing socket addr", port->conf->name);
	rc = my_sock_set_reuseaddr(MY_SOURCE(port)->fd);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error reusing socket addr (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_set_reuseaddr;
	}

	my_core_event_handler_add(port->core, MY_SOURCE(port)->fd, my_source_udp_event_handler, port);

	return 0;

_MY_ERR_set_reuseaddr:
_MY_ERR_set_nonblock:
_MY_ERR_set_recv_buffer_size:
_MY_ERR_mcast_join:
_MY_ERR_sock_bind:
_MY_ERR_sock_create:
	return -1;
}

static int my_source_udp_close(my_port_t *port)
{
	int rc;

	my_core_event_handler_del(port->core, MY_SOURCE(port)->fd);

	MY_DEBUG("core/%s: leaving mcast group", port->conf->name);
	rc = my_net_mcast_leave(MY_SOURCE(port)->fd, MY_SOURCE(port)->sa_local, MY_SOURCE(port)->sa_group);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error joining mcast group (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_mcast_leave;
	}

	MY_DEBUG("core/%s: closing socket", port->conf->name);
	rc = my_sock_close(MY_SOURCE(port)->fd);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error closing file (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_sock_close;
	}

	return 0;

_MY_ERR_sock_close:
_MY_ERR_mcast_leave:
	return rc;
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
