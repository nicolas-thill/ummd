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
#include <string.h>
#include <net/if.h>
#include <linux/sockios.h>

#include "util/net.h"

#include "util/log.h"


int my_net_get_interface_addr(int fd, char *if_name, struct sockaddr *sa)
{
	struct ifreq ifr;
	int rc;

	ifr.ifr_addr.sa_family = sa->sa_family;
	strncpy(ifr.ifr_name, if_name, IFNAMSIZ - 1);

	rc = ioctl(fd, SIOCGIFADDR, &ifr);
	if (rc < 0) {
		my_log(MY_LOG_ERROR, "util/net: error getting address for interface '%s' (%d: %s)", if_name, errno, strerror(errno));
		goto _ERR_ioctl;
	}

	if (sa->sa_family == AF_INET) {
		memcpy(&(((struct sockaddr_in *)sa)->sin_addr), &(ifr.ifr_addr), sizeof(struct in_addr));
	}

#if HAVE_IPV6
	if (sa->sa_family == AF_INET6) {
		memcpy(&(((struct sockaddr_in6 *)sa)->sin6_addr), &(ifr.ifr_addr), sizeof(struct in6_addr));
	}
#endif

	return 0;

_ERR_ioctl:
	return rc;
}


static my_net4_addr_is_multicast(struct sockaddr *sa)
{
	return IN_MULTICAST(ntohl(((struct sockaddr_in *)sa)->sin_addr.s_addr));
}

static my_net6_addr_is_multicast(struct sockaddr *sa)
{
	return IN6_IS_ADDR_MULTICAST(&((struct sockaddr_in6 *)sa)->sin6_addr);
}

int my_net_addr_is_multicast(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return my_net4_addr_is_multicast(sa);
	}

#ifdef HAVE_IPV6
	if (sa->sa_family == AF_INET6) {
		return my_net6_addr_is_multicast(sa)
	}
#endif

	return -1;
}


static int my_net4_set_multicast_ttl(int fd, int ttl)
{
	return setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
}

static int my_net6_set_multicast_ttl(int fd, int ttl)
{
	return setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &ttl, sizeof(ttl));
}

int my_net_set_multicast_ttl(int fd, int ttl, struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return my_net4_set_multicast_ttl(fd, ttl);
	}

#if HAVE_IPV6
	if (sa->sa_family == AF_INET6) {
		return my_net6_set_multicast_ttl(fd, ttl);
	}
#endif

	return -1;
}


static int my_net4_set_multicast_membership(int fd, int op, struct sockaddr *sa_local, struct sockaddr *sa_group)
{
	struct ip_mreq mr;

/*
	mr.imr_interface.s_addr = ((struct sockaddr_in *)sa_local)->sin_addr.s_addr;
	mr.imr_multiaddr.s_addr = ((struct sockaddr_in *)sa_group)->sin_addr.s_addr;
*/
	memcpy(&(mr.imr_interface), &(((struct sockaddr_in *)sa_local)->sin_addr), sizeof(struct in_addr));
	memcpy(&(mr.imr_multiaddr), &(((struct sockaddr_in *)sa_group)->sin_addr), sizeof(struct in_addr));

	return setsockopt(fd, IPPROTO_IP, op, (const void *)&mr, sizeof(mr));
}

static int my_net6_set_multicast_membership(int fd, int op, struct sockaddr *sa_local, struct sockaddr *sa_group)
{
	struct ipv6_mreq mr;

	memcpy(&(mr.ipv6mr_interface), &(((struct sockaddr_in6 *)sa_local)->sin6_addr), sizeof(struct in6_addr));
	memcpy(&(mr.ipv6mr_multiaddr), &(((struct sockaddr_in6 *)sa_group)->sin6_addr), sizeof(struct in6_addr));

	return setsockopt(fd, IPPROTO_IPV6, op, &mr, sizeof(mr));
}

int my_net_join_multicast(int fd, struct sockaddr *sa, struct sockaddr *sa_group)
{
	if (sa->sa_family == AF_INET) {
		return my_net4_set_multicast_membership(fd, IP_ADD_MEMBERSHIP, sa, sa_group);
	}

#if HAVE_IPV6
	if (sa->sa_family == AF_INET6) {
		return my_net6_set_multicast_membership(fd, IPV6_ADD_MEMBERSHIP, sa, sa_group);
	}
#endif

	return -1;
}

int my_net_leave_multicast(int fd, struct sockaddr *sa, struct sockaddr *sa_group)
{
	if (sa->sa_family == AF_INET) {
		return my_net4_set_multicast_membership(fd, IP_DROP_MEMBERSHIP, sa, sa_group);
	}

#if HAVE_IPV6
	if (sa->sa_family == AF_INET6) {
		return my_net6_set_multicast_membership(fd, IPV6_DROP_MEMBERSHIP, sa, sa_group);
	}
#endif

	return -1;
}
