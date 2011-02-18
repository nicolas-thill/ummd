/*
 *  ummd ( Micro MultiMedia Daemon )
 *
 *  Copyright (C) 2011 Nicolas Thill <nicolas.thill@gmail.com>
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

#ifndef __MY_UTIL_NET_H
#define __MY_UTIL_NET_H

#include <netinet/in.h>

extern int my_net_addr_get(int fd, char *if_name, struct sockaddr *sa);

extern int my_net_addr_is_multicast(struct sockaddr *sa);


extern int my_net_mcast_set_interface(int fd, struct sockaddr *sa);
extern int my_net_mcast_set_loopback(int fd, struct sockaddr *sa, int loop);
extern int my_net_mcast_set_ttl(int fd, struct sockaddr *sa, int ttl);

extern int my_net_mcast_join(int fd, struct sockaddr *sa, struct sockaddr *sa_group);
extern int my_net_mcast_leave(int fd, struct sockaddr *sa, struct sockaddr *sa_group);


extern int my_sock_create(int family, int type);
extern int my_sock_close(int fd);

extern int my_sock_bind(int fd, struct sockaddr *sa);

extern int my_sock_set_rcv_buffer_size(int fd, int size);
extern int my_sock_set_snd_buffer_size(int fd, int size);

extern int my_sock_set_nonblock(int fd);
extern int my_sock_set_reuseaddr(int fd);

#endif /* __MY_UTIL_NET_H */
