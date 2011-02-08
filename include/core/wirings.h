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

#ifndef __MY_WIRINGS_H
#define __MY_WIRINGS_H

#include "core.h"

#include "core/ports.h"

typedef struct my_wiring_s my_wiring_t;
typedef struct my_wiring_conf_s my_wiring_conf_t;

struct my_wiring_s {
	my_core_t *core;
	my_port_t *source;
	my_port_t *target;
};

struct my_wiring_conf_s {
	int index;
	char *name;
	char *source;
	char *target;
};

#define MY_WIRING(p) ((my_wiring_t *)(p))
#define MY_WIRING_CONF(p) ((my_wiring_conf_t *)(p))

extern int my_wiring_create_all(my_core_t *core, my_conf_t *conf);
extern void my_wiring_destroy_all(my_core_t *core);

#endif /* __MY_WIRINGS_H */
