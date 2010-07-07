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

#include "util/prop.h"

#include "util/mem.h"

typedef struct my_prop_s my_prop_t;

struct my_prop_s {
	char *name;
	char *value;
};

#define MY_PROP(p) ((my_prop_t *)(p))

static my_prop_t *my_prop_priv_find(my_list_t *list, char *name)
{
	my_node_t *node;
	my_prop_t *prop;

	for (node = list->head; node; node = node->next) {
		prop = MY_PROP(node->data);
		if (strcmp(prop->name, name) == 0) {
			return prop;
		}
	}

	return NULL;
}

int my_prop_add(my_list_t *list, char *name, char *value)
{
	my_prop_t *prop;
	char *p;

	prop = my_prop_priv_find(list, name);
	if (prop) {
		p = strdup(value);
		if (!p) {
			return -1;
		}
		free(prop->value);
		prop->value = p;
		return 0;
	}

	prop = my_mem_alloc(sizeof(*prop));
	if (!prop) {
		goto _MY_ERR_alloc;
	}
	p = strdup(name);
	if (!p) {
		goto _MY_ERR_set_name;
	}
	prop->name = p;
	p = strdup(value);
	if (!p) {
		goto _MY_ERR_set_value;
	}
	if (my_list_enqueue(list, prop) != 0) {
		goto _MY_ERR_add;
	}

	return 0;

	my_list_dequeue(list);
_MY_ERR_add:
	free(prop->value);
_MY_ERR_set_value:
	free(prop->name);
_MY_ERR_set_name:
	my_mem_free(prop);
_MY_ERR_alloc:
	return -1;
}

char *my_prop_lookup(my_list_t *list, char *name)
{
	my_prop_t *prop;

	prop = my_prop_priv_find(list, name);
	if (prop) {
		return prop->value;
	}

	return NULL;
}

void my_prop_purge(my_list_t *list)
{
	my_prop_t *prop;

	while (prop = my_list_dequeue(list)) {
		free(prop->name);
		free(prop->value);
		my_mem_free(prop);
	}
}
