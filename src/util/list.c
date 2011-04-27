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

void my_list_insert_before(my_list_t *list, my_node_t *node, void *data)
{
	my_node_t *new_node;

	new_node = my_mem_alloc(sizeof(*new_node));
	if (!new_node)
		return;

	new_node->data = data;

	if (!node->prev) {
		list->head = new_node;
		new_node->prev = NULL;
	} else {
		new_node->prev = node->prev;
		node->prev->next = new_node;
	}

	new_node->next = node;
	node->prev = new_node;
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

static my_node_t *my_list_remove_head(my_list_t *list)
{
	my_node_t *node = list->head;

	if (node) {
		if (node->next) {
			node->next->prev = NULL;
			list->head = node->next;
		} else {
			list->head = list->tail = NULL;
		}
	}

	return node;
}

static my_node_t *my_list_remove_tail(my_list_t *list)
{
	my_node_t *node = list->tail;

	if (node) {
		if (node->prev) {
			node->prev->next = NULL;
			list->tail = node->prev;
		} else {
			list->tail = list->head = NULL;
		}
	}

	return node;
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

int my_list_is_empty(my_list_t *list)
{
	return (list->head == NULL);
}

void *my_list_dequeue_head(my_list_t *list)
{
	my_node_t *node;
	void *data;

	node = my_list_remove_head(list);
	if (node) {
		data = node->data;
		my_mem_free(node);
		return data;
	}

	return NULL;
}

void *my_list_dequeue_tail(my_list_t *list)
{
	my_node_t *node;
	void *data;

	node = my_list_remove_tail(list);
	if (node) {
		data = node->data;
		my_mem_free(node);
		return data;
	}

	return NULL;
}

void my_list_remove(my_list_t *list, my_node_t *node)
{
	if (node->next) {
		node->next->prev = node->prev;
	} else {
		list->tail = node->prev;
	}
	if (node->prev) {
		node->prev->next = node->next;
	} else {
		list->head = node->next;
	}
	my_mem_free(node);
}

int my_list_enqueue_head(my_list_t *list, void *data)
{
	my_node_t *node;

	node = my_mem_alloc(sizeof(*node));
	if (node) {
		my_list_add_head(list, node);
		node->data = data;
		return 0;
	}

	return -1;
}

int my_list_enqueue_tail(my_list_t *list, void *data)
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

void my_list_purge(my_list_t *list, int flags)
{
	void *data;

	while (data = my_list_dequeue(list)) {
		if (flags & MY_LIST_PURGE_FLAG_FREE_DATA) {
			my_mem_free(data);
		}
	}
}


