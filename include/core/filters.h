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

#ifndef __MY_FILTERS_H
#define __MY_FILTERS_H

#include "autoconf.h"

#include "core.h"

typedef enum {
	MY_FILTER_NULL,
	MY_FILTER_DELAY,
} my_filter_id_t;

typedef struct my_filter my_filter_t;
typedef struct my_filter_conf my_filter_conf_t;
typedef struct my_filter_impl my_filter_impl_t;

typedef my_filter_t *(*my_filter_create_fn_t)(my_filter_conf_t *conf);
typedef void (*my_filter_destroy_fn_t)(my_filter_t *filter);

#define MY_FILTER(p) ((my_filter_t *)(p))
#define MY_FILTER_CONF(p) ((my_filter_conf_t *)(p))
#define MY_FILTER_IMPL(p) ((my_filter_impl_t *)(p))

struct my_filter {
	my_core_t *core;
	my_filter_conf_t *conf;
	my_filter_impl_t *impl;
};

struct my_filter_conf {
	int index;
	char *name;
	char *desc;
	char *type;
	char *arg;
};

struct my_filter_impl {
	my_filter_id_t id;
	char *name;
	char *desc;
	my_filter_create_fn_t create;
	my_filter_destroy_fn_t destroy;
};

extern int my_filter_create_all(my_core_t *core, my_conf_t *conf);
extern int my_filter_destroy_all(my_core_t *core);

extern void my_filter_register_all(void);

#ifdef MY_DEBUGGING
extern void my_filter_dump_all(void);
#endif

#endif /* __MY_FILTERS_H */
