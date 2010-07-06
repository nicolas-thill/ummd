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

#ifndef __MY_TARGETS_H
#define __MY_TARGETS_H

#include "autoconf.h"

#include "core.h"

#include "util/list.h"

typedef struct my_target_s my_target_t;
typedef struct my_target_conf_s my_target_conf_t;

struct my_target_s {
	my_core_t *core;
	my_target_conf_t *conf;
	my_list_t *iports;
};

struct my_target_conf_s {
	int index;
	char *name;
	char *desc;
	char *type;
	char *url;
};

#define MY_TARGET(p) ((my_target_t *)(p))
#define MY_TARGET_CONF(p) ((my_target_conf_t *)(p))

#define MY_TARGET_GET_CORE(p) (MY_TARGET(p)->core)
#define MY_TARGET_GET_CONF(p) (MY_TARGET(p)->conf)

extern my_target_t *my_target_create(my_target_conf_t *conf);
extern void my_target_destroy(my_target_t *target);

extern int my_target_create_all(my_core_t *core, my_conf_t *conf);
extern int my_target_destroy_all(my_core_t *core);

extern int my_target_open_all(my_core_t *core);
extern int my_target_close_all(my_core_t *core);

extern void my_target_register_all(void);

#ifdef MY_DEBUGGING
extern void my_target_dump_all(void);
#endif

#endif /* __MY_TARGETS_H */
