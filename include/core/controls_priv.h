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

#ifndef __MY_CONTROLS_PRIV_H
#define __MY_CONTROLS_PRIV_H

#include "core/controls.h"

typedef struct my_control_impl_s my_control_impl_t;
typedef struct my_control_priv_s my_control_priv_t;

typedef my_control_t *(*my_control_create_fn_t)(my_core_t *core, my_control_conf_t *conf);
typedef void (*my_control_destroy_fn_t)(my_control_t *control);

typedef int (*my_control_open_fn_t)(my_control_t *control);
typedef int (*my_control_close_fn_t)(my_control_t *control);

struct my_control_impl_s {
	char *name;
	char *desc;
	my_control_create_fn_t create;
	my_control_destroy_fn_t destroy;
	my_control_open_fn_t open;
	my_control_close_fn_t close;
};

struct my_control_priv_s {
	my_control_t _inherited;
	my_control_impl_t *impl;
};

#define MY_CONTROL_IMPL(p) ((my_control_impl_t *)(p))
#define MY_CONTROL_PRIV(p) ((my_control_priv_t *)(p))

#define MY_CONTROL_GET_IMPL(p) (MY_CONTROL_PRIV(p)->impl)


extern my_control_t *my_control_create(my_core_t *core, my_control_conf_t *conf, size_t size);
extern void my_control_destroy(my_control_t *control);

extern int my_control_create_all(my_core_t *core, my_conf_t *conf);
extern int my_control_destroy_all(my_core_t *core);

extern int my_control_open_all(my_core_t *core);
extern int my_control_close_all(my_core_t *core);

extern void my_control_register_all(void);

#ifdef MY_DEBUGGING
extern void my_control_dump_all(void);
#endif

#endif /* __MY_CONTROLS_PRIV_H */
