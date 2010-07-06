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

#ifndef __MY_CONTROLS_H
#define __MY_CONTROLS_H

#include "autoconf.h"

#include "core.h"

typedef struct my_control_s my_control_t;
typedef struct my_control_conf_s my_control_conf_t;

struct my_control_s {
	my_core_t *core;
	my_control_conf_t *conf;
};

struct my_control_conf_s {
	int index;
	char *name;
	char *desc;
	char *type;
	char *url;
};

#define MY_CONTROL(p) ((my_control_t *)(p))
#define MY_CONTROL_CONF(p) ((my_control_conf_t *)(p))

#define MY_CONTROL_GET_CORE(p) (MY_CONTROL(p)->core)
#define MY_CONTROL_GET_CONF(p) (MY_CONTROL(p)->conf)

extern my_control_t *my_control_create(my_control_conf_t *conf);
extern void my_control_destroy(my_control_t *control);

extern int my_control_create_all(my_core_t *core, my_conf_t *conf);
extern int my_control_destroy_all(my_core_t *core);

extern int my_control_open_all(my_core_t *core);
extern int my_control_close_all(my_core_t *core);

extern void my_control_register_all(void);

#ifdef MY_DEBUGGING
extern void my_control_dump_all(void);
#endif

#endif /* __MY_CONTROLS_H */
