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

#include <stdlib.h>

#include "util/list.h"

#include "util/mem.h"

static void my_list_init(my_list_t *list) {
	list->head = list->tail = NULL;
}

static void my_list_add_head(my_list_t *list, my_node_t *node)
{
	if (list->head) {
		node->next = list->head;
		list->head->prev = node;
		list->head = node;
	} else {
		list->head = list->tail = node;
	}
}

static void my_list_add_tail(my_list_t *list, my_node_t *node)
{
	if (list->tail) {
		node->prev = list->tail;
		list->tail->next = node;
		list->tail = node;
	} else {
		list->tail = list->head = node;
	}
}

my_list_t *my_list_create(void)
{
	my_list_t *list;
	
	list = my_mem_alloc(sizeof(*list));
	
	return list;
}

void my_list_destroy(my_list_t *list)
{
	my_mem_free(list);
}

void *my_list_get(my_list_t *list, int n)
{
	my_node_t *node = list->head;
	int i = 0;

	while (node) {
		if (i == n) {
			return node->data;
		}
		node = node->next;
		i++;
	}

	return NULL;
}

int my_list_queue(my_list_t *list, void *data)
{
	my_node_t *node;
	
	node = my_mem_alloc(sizeof(*node));
	if (node) {
		my_list_add_tail(list, node);
		node->data = data;
		return 0;
	}
	
	return -1;
}

int my_list_iter(my_list_t *list, my_list_iter_fn_t func, void *user)
{
	my_node_t *node;
	int flags;
	int rc;

	flags = MY_LIST_ITER_FLAG_FIRST;
	for (node = list->head; node; node = node->next) {
		if (!(node->next)) {
			flags |= MY_LIST_ITER_FLAG_LAST;
		}
		rc = (func)(node->data, user, flags);
		if (rc) {
			return rc;
		}
		flags &= ~MY_LIST_ITER_FLAG_FIRST;
	}

	return 0;
}
