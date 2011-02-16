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

extern int my_net_get_interface_addr(int fd, char *if_name, struct sockaddr *sa);

extern int my_net_addr_is_multicast(struct sockaddr *sa);

extern int my_net_set_multicast_ttl(int fd, int ttl, struct sockaddr *sa);

extern int my_net_join_multicast(int fd, struct sockaddr *sa, struct sockaddr *sa_group);
extern int my_net_leave_multicast(int fd, struct sockaddr *sa, struct sockaddr *sa_group);


#endif /* __MY_UTIL_NET_H */
