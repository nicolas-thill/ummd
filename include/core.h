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

#ifndef __MY_CORE_H
#define __MY_CORE_H

#include <sys/time.h>

#include "autoconf.h"

#include "conf.h"

#include "util/list.h"

typedef struct my_core_s my_core_t;

struct my_core_s {
	my_list_t *controls;
	my_list_t *filters;
	my_list_t *sources;
	my_list_t *targets;
	my_list_t *wirings;
};

#define MY_CORE(p) ((my_core_t *)(p))


extern my_core_t *my_core_create(void);
extern void my_core_destroy(my_core_t *core);

extern int my_core_init(my_core_t *core, my_conf_t *conf);
extern void my_core_loop(my_core_t *core);
extern void my_core_stop(my_core_t *core);

typedef int (*my_event_handler_t)(int fd, void *p);
typedef int (*my_alarm_handler_t)(void *p);

extern int my_core_alarm_add(my_core_t *core, unsigned int timeout,
			     int reoccurring, my_alarm_handler_t handler, void *p);
extern int my_core_event_handler_add(my_core_t *core, int fd, my_event_handler_t handler, void *p);
extern int my_core_event_handler_del(my_core_t *core, int fd);

extern int my_core_handle_command(my_core_t *core, void *buf, int len);

#ifdef MY_DEBUGGING
extern void my_core_dump(my_core_t *core);
#endif

#endif /* __MY_CORE_H */
