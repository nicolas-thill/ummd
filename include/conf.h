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

#ifndef __MY_CONF_H
#define __MY_CONF_H

#include "autoconf.h"

#include "util/list.h"

typedef struct my_conf my_conf_t;

struct my_conf {
	char *cfg_file;
	char *log_file;
	char *pid_file;
	int log_level;
	my_list_t *controls;
	my_list_t *filters;
	my_list_t *sources;
	my_list_t *targets;
	my_list_t *wirings;
};

extern my_conf_t * my_conf_create(void);
void my_conf_destroy(my_conf_t *conf);

extern int my_conf_parse(my_conf_t *conf);

#ifdef MY_DEBUGGING
extern void my_conf_dump(my_conf_t *conf);
#endif

#endif /* __MY_CONF_H */
