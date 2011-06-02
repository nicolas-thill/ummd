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

typedef struct my_udp_priv_s my_udp_priv_t;

struct my_udp_priv_s {
	my_dport_t _inherited;
	char *ip_addr;
	int ip_port;
	int fd;
	int sa_len;
	struct sockaddr *sa_local;
	struct sockaddr *sa_group;
};

#define MY_UDP(p) ((my_udp_priv_t *)(p))
#define MY_UDP_SIZE (sizeof(my_udp_priv_t))

static int my_source_udp_event_handler(int fd, void *p)
{
	my_udp_priv_t *source = MY_UDP(p);

	/* do something */
	return 0;
}

static int my_target_udp_event_handler(int fd, void *p)
{
	my_udp_priv_t *source = MY_UDP(p);

	/* do something */
	return 0;
}

static my_port_t *my_io_udp_create(my_core_t *core, my_port_conf_t *conf)
{
	my_port_t *port;
	char *prop;
	struct in_addr ia;
	int ret;

	port = my_port_create_priv(MY_UDP_SIZE);
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
	MY_UDP(port)->ip_addr = prop;

	prop = my_prop_lookup(conf->properties, "port");
	if (!prop) {
		my_log(MY_LOG_ERROR, "core/%s: missing 'port' property", port->conf->name);
		goto _MY_ERR_conf;
	}
	MY_UDP(port)->ip_port = atoi(prop);
	MY_UDP(port)->sa_len = sizeof(struct sockaddr_in);

	MY_UDP(port)->sa_local = my_mem_alloc(MY_UDP(port)->sa_len);
	if (!MY_UDP(port)->sa_local) {
		goto _MY_ERR_alloc_sa_local;
	}
	my_mem_zero(MY_UDP(port)->sa_local, MY_UDP(port)->sa_len);

	MY_UDP(port)->sa_group = my_mem_alloc(MY_UDP(port)->sa_len);
	if (!MY_UDP(port)->sa_group) {
		goto _MY_ERR_alloc_sa_group;
	}
	my_mem_zero(MY_UDP(port)->sa_group, MY_UDP(port)->sa_len);

	((struct sockaddr_in *)MY_UDP(port)->sa_local)->sin_family = AF_INET;
	((struct sockaddr_in *)MY_UDP(port)->sa_local)->sin_addr.s_addr = htonl(INADDR_ANY);
	((struct sockaddr_in *)MY_UDP(port)->sa_local)->sin_port = htons(MY_UDP(port)->ip_port);

	((struct sockaddr_in *)MY_UDP(port)->sa_group)->sin_family = AF_INET;
	((struct sockaddr_in *)MY_UDP(port)->sa_group)->sin_addr.s_addr = inet_addr(MY_UDP(port)->ip_addr);
	((struct sockaddr_in *)MY_UDP(port)->sa_group)->sin_port = htons(MY_UDP(port)->ip_port);

	return port;

	my_mem_free(MY_UDP(port)->sa_group);
_MY_ERR_alloc_sa_group:
	my_mem_free(MY_UDP(port)->sa_local);
_MY_ERR_alloc_sa_local:
_MY_ERR_conf:
	my_port_destroy_priv(port);
_MY_ERR_create_priv:
	return NULL;
}

static void my_io_udp_destroy(my_port_t *port)
{
	my_mem_free(MY_UDP(port)->sa_group);
	my_mem_free(MY_UDP(port)->sa_local);
	my_port_destroy_priv(port);
}

static int my_io_udp_open(my_port_t *port)
{
	int rc;

	MY_DEBUG("core/%s: opening socket", port->conf->name);
	MY_UDP(port)->fd = my_sock_create(PF_INET, SOCK_DGRAM);
	if (MY_UDP(port)->fd < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error opening socket' (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_sock_create;
	}

	MY_DEBUG("core/%s: binding socket", port->conf->name);
	rc = my_sock_bind(MY_UDP(port)->fd, MY_UDP(port)->sa_local);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error binding socket socket (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_sock_bind;
	}

	MY_DEBUG("core/%s: setting socket send buffer to %d", port->conf->name, 65535);
	rc = my_sock_set_snd_buffer_size(MY_UDP(port)->fd, 65535);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error setting socket send buffer (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_set_snd_buffer_size;
	}

	MY_DEBUG("core/%s: setting socket receive buffer to %d", port->conf->name, 65535);
	rc = my_sock_set_rcv_buffer_size(MY_UDP(port)->fd, 65535);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error setting socket receive buffer (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_set_rcv_buffer_size;
	}

	MY_DEBUG("core/%s: putting socket in non-blocking mode", port->conf->name);
	rc = my_sock_set_nonblock(MY_UDP(port)->fd);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error putting socket in non-blocking mode (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_set_nonblock;
	}

	MY_DEBUG("core/%s: reusing socket addr", port->conf->name);
	rc = my_sock_set_reuseaddr(MY_UDP(port)->fd);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error reusing socket addr (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_set_reuseaddr;
	}

	MY_DEBUG("core/%s: joining mcast group", port->conf->name);
	rc = my_net_mcast_join(MY_UDP(port)->fd, MY_UDP(port)->sa_local, MY_UDP(port)->sa_group);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error joining mcast group (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_mcast_join;
	}

	my_core_event_handler_add(port->core, MY_UDP(port)->fd, MY_PORT_GET_IMPL(port)->handler, port);
	return 0;

_MY_ERR_mcast_join:
_MY_ERR_set_reuseaddr:
_MY_ERR_set_nonblock:
_MY_ERR_set_snd_buffer_size:
_MY_ERR_set_rcv_buffer_size:
_MY_ERR_sock_bind:
_MY_ERR_sock_create:
	return -1;
}

static int my_io_udp_close(my_port_t *port)
{
	int rc;

	my_core_event_handler_del(port->core, MY_UDP(port)->fd);

	MY_DEBUG("core/%s: leaving mcast group", port->conf->name);
	rc = my_net_mcast_leave(MY_UDP(port)->fd, MY_UDP(port)->sa_local, MY_UDP(port)->sa_group);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error leaving mcast group (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_mcast_leave;
	}

	MY_DEBUG("core/%s: closing socket", port->conf->name);
	rc = my_sock_close(MY_UDP(port)->fd);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "core/%s: error closing file (%d: %s)", port->conf->name, errno, strerror(errno));
		goto _MY_ERR_sock_close;
	}

	return 0;

_MY_ERR_sock_close:
_MY_ERR_mcast_leave:
	return rc;
}

static int my_io_udp_get(my_port_t *port, void *buf, int len)
{
	int n;

	n = read(MY_UDP(port)->fd, buf, len);
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

static int my_io_udp_put(my_port_t *port, void *buf, int len)
{
	int n;

	n = sendto(MY_UDP(port)->fd, buf, len, 0, MY_UDP(port)->sa_group, MY_UDP(port)->sa_len);
	if (n < 0) {
		if ((errno == EWOULDBLOCK) || (errno == EINTR)) {
			n = 0;
			goto out;
		}

		my_log(MY_LOG_ERROR, "core/%s: error sending to socket (%d: %s)", port->conf->name, errno, strerror(errno));
		return n;
	}

out:
	MY_DEBUG("core/%s: wrote %d bytes to socket", port->conf->name, n);
	return n;
}

my_port_impl_t my_source_udp = {
	.name = "udp",
	.desc = "UDP multicast source",
	.create = my_io_udp_create,
	.destroy = my_io_udp_destroy,
	.open = my_io_udp_open,
	.close = my_io_udp_close,
	.get = my_io_udp_get,
	.handler = my_source_udp_event_handler,
};

my_port_impl_t my_target_udp = {
	.name = "udp",
	.desc = "UDP multicast target",
	.create = my_io_udp_create,
	.destroy = my_io_udp_destroy,
	.open = my_io_udp_open,
	.close = my_io_udp_close,
	.put = my_io_udp_put,
	.handler = my_target_udp_event_handler,
};
