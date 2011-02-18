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
#include <string.h>
#include <net/if.h>
#include <linux/sockios.h>

#include "util/net.h"

#include "util/log.h"


int my_net_addr_get(int fd, char *if_name, struct sockaddr *sa)
{
	struct ifreq ifr;
	int rc;

	ifr.ifr_addr.sa_family = sa->sa_family;
	strncpy(ifr.ifr_name, if_name, IFNAMSIZ - 1);

	rc = ioctl(fd, SIOCGIFADDR, &ifr);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "util/net: error getting address for interface '%s' (%d: %s)", if_name, errno, strerror(errno));
		goto _MY_ERR_ioctl;
	}

	if (sa->sa_family == AF_INET) {
		memcpy(&(((struct sockaddr_in *)sa)->sin_addr), &(ifr.ifr_addr), sizeof(struct in_addr));
	}
#if HAVE_IPV6
	else if (sa->sa_family == AF_INET6) {
		memcpy(&(((struct sockaddr_in6 *)sa)->sin6_addr), &(ifr.ifr_addr), sizeof(struct in6_addr));
	}
#endif

	return 0;

_MY_ERR_ioctl:
	return rc;
}


static my_net_addr_is_multicast_4(struct sockaddr *sa)
{
	return IN_MULTICAST(ntohl(((struct sockaddr_in *)sa)->sin_addr.s_addr));
}

static my_net_addr_is_multicast_6(struct sockaddr *sa)
{
	return IN6_IS_ADDR_MULTICAST(&((struct sockaddr_in6 *)sa)->sin6_addr);
}

int my_net_addr_is_multicast(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return my_net_addr_is_multicast_4(sa);
	}
#ifdef HAVE_IPV6
	else if (sa->sa_family == AF_INET6) {
		return my_net_addr_is_multicast_6(sa)
	}
#endif

	return -1;
}


int my_net_mcast_set_interface(int fd, struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &(((struct sockaddr_in *)sa)->sin_addr), sizeof(struct in_addr));
	}
#if HAVE_IPV6
	else if (sa->sa_family == AF_INET6) {
		return setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &(((struct sockaddr_in6 *)sa)->sin6_addr), sizeof(struct in6_addr));
	}
#endif

	return -1;
}


int my_net_mcast_set_loopback(int fd, struct sockaddr *sa, int loop)
{
	u_int8_t op = loop;

	if (sa->sa_family == AF_INET) {
		return setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &op, sizeof(op));
	}
#if HAVE_IPV6
	else if (sa->sa_family == AF_INET6) {
		return setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &op, sizeof(op));
	}
#endif

	return -1;
}


int my_net_mcast_set_ttl(int fd, struct sockaddr *sa, int ttl)
{
	u_int8_t op = ttl;

	if (sa->sa_family == AF_INET) {
		return setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &op, sizeof(op));
	}
#if HAVE_IPV6
	else if (sa->sa_family == AF_INET6) {
		return setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &op, sizeof(op));
	}
#endif

	return -1;
}


static int my_net_mcast_set_membership_4(int fd, int op, struct sockaddr *sa_local, struct sockaddr *sa_group)
{
	struct ip_mreq mr;

	mr.imr_interface.s_addr = ((struct sockaddr_in *)sa_local)->sin_addr.s_addr;
	mr.imr_multiaddr.s_addr = ((struct sockaddr_in *)sa_group)->sin_addr.s_addr;
/*
	memcpy(&(mr.imr_interface), &(((struct sockaddr_in *)sa_local)->sin_addr), sizeof(struct in_addr));
	memcpy(&(mr.imr_multiaddr), &(((struct sockaddr_in *)sa_group)->sin_addr), sizeof(struct in_addr));
*/

	return setsockopt(fd, IPPROTO_IP, op, (const void *)&mr, sizeof(mr));
}

static int my_net_mcast_set_membership_6(int fd, int op, struct sockaddr *sa_local, struct sockaddr *sa_group)
{
	struct ipv6_mreq mr;

	memcpy(&(mr.ipv6mr_interface), &(((struct sockaddr_in6 *)sa_local)->sin6_addr), sizeof(struct in6_addr));
	memcpy(&(mr.ipv6mr_multiaddr), &(((struct sockaddr_in6 *)sa_group)->sin6_addr), sizeof(struct in6_addr));

	return setsockopt(fd, IPPROTO_IPV6, op, &mr, sizeof(mr));
}

int my_net_mcast_join(int fd, struct sockaddr *sa, struct sockaddr *sa_group)
{
	if (sa->sa_family == AF_INET) {
		return my_net_mcast_set_membership_4(fd, IP_ADD_MEMBERSHIP, sa, sa_group);
	}
#if HAVE_IPV6
	else if (sa->sa_family == AF_INET6) {
		return my_net_mcast_set_membership_6(fd, IPV6_ADD_MEMBERSHIP, sa, sa_group);
	}
#endif

	return -1;
}

int my_net_mcast_leave(int fd, struct sockaddr *sa, struct sockaddr *sa_group)
{
	if (sa->sa_family == AF_INET) {
		return my_net_mcast_set_membership_4(fd, IP_DROP_MEMBERSHIP, sa, sa_group);
	}
#if HAVE_IPV6
	else if (sa->sa_family == AF_INET6) {
		return my_net_mcast_set_membership_6(fd, IPV6_DROP_MEMBERSHIP, sa, sa_group);
	}
#endif

	return -1;
}


int my_sock_create(int family, int type)
{
	return socket(family, type, 0);
}

int my_sock_close(int fd)
{
	return close(fd);
}

int my_sock_bind(int fd, struct sockaddr *sa)
{
	int sa_len;

	if (sa->sa_family == AF_INET) {
		sa_len = sizeof(struct sockaddr_in);
	}
#ifdef HAVE_IPV6
	else if (sa->sa_family == AF_INET6) {
		sa_len = sizeof(struct sockaddr_in6);
	}
#endif

	return bind(fd, sa, sa_len);
}


int my_sock_set_rcv_buffer_size(int fd, int size)
{
	int so = size;

	return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &so, sizeof(so));
}

int my_sock_set_snd_buffer_size(int fd, int size)
{
	int so = size;

	return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &so, sizeof(so));
}


int my_sock_set_reuseaddr(int fd)
{
	int so = 1;

	return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &so, sizeof(so));
}

int my_sock_set_nonblock(int fd)
{
	int so;

	so = fcntl(fd, F_GETFL, 0);
	if (so < 0) {
		return so;
	}

	return fcntl(fd, F_SETFL, so | O_NONBLOCK);
}
