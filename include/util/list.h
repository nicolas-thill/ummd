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

#ifndef __MY_UTIL_LIST_H
#define __MY_UTIL_LIST_H

typedef struct my_list my_list_t;
typedef struct my_node my_node_t;

struct my_list {
	my_node_t *head, *tail;
};

struct my_node {
	my_node_t *prev, *next;
	void *data;
};

#define MY_LIST_ITER_FLAG_FIRST  0x0001
#define MY_LIST_ITER_FLAG_LAST   0x0002

typedef int (*my_list_iter_fn_t)(void *data, void *user, int flags);


extern my_list_t *my_list_create(void);
extern void my_list_destroy(my_list_t *list);

extern void *my_list_get(my_list_t *list, int n);
extern int my_list_iter(my_list_t *list, my_list_iter_fn_t func, void *user);

extern int my_list_enqueue(my_list_t *list, void *data);

#endif /* __MY_UTIL_LIST_H */
