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

#ifndef __MY_SOURCES_PRIV_H
#define __MY_SOURCES_PRIV_H

#include "core/sources.h"

typedef struct my_source_impl_s my_source_impl_t;
typedef struct my_source_priv_s my_source_priv_t;

typedef my_source_t *(*my_source_create_fn_t)(my_core_t *core, my_source_conf_t *conf);
typedef void (*my_source_destroy_fn_t)(my_source_t *source);

typedef int (*my_source_open_fn_t)(my_source_t *source);
typedef int (*my_source_close_fn_t)(my_source_t *source);

struct my_source_impl_s {
	char *name;
	char *desc;
	my_source_create_fn_t create;
	my_source_destroy_fn_t destroy;
	my_source_open_fn_t open;
	my_source_close_fn_t close;
};

struct my_source_priv_s {
	my_source_t _inherited;
	my_source_impl_t *impl;
};

#define MY_SOURCE_IMPL(p) ((my_source_impl_t *)(p))
#define MY_SOURCE_PRIV(p) ((my_source_priv_t *)(p))

#define MY_SOURCE_GET_IMPL(p) (MY_SOURCE_PRIV(p)->impl)


extern my_source_t *my_source_create(my_core_t *core, my_source_conf_t *conf, size_t size);
extern void my_source_destroy(my_source_t *source);

extern int my_source_create_all(my_core_t *core, my_conf_t *conf);
extern int my_source_destroy_all(my_core_t *core);

extern int my_source_open_all(my_core_t *core);
extern int my_source_close_all(my_core_t *core);

extern void my_source_register_all(void);

#ifdef MY_DEBUGGING
extern void my_source_dump_all(void);
#endif


#endif /* __MY_SOURCES_PRIV_H */
