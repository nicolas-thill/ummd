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

#ifndef __MY_UTIL_PROP_H
#define __MY_UTIL_PROP_H

#include "util/list.h"

extern int my_prop_add(my_list_t *list, char *name, char *value);

extern char *my_prop_lookup(my_list_t *list, char *name);

extern void my_prop_purge(my_list_t *list);


typedef int (*my_prop_iter_fn_t)(char *name, char *value, void *user, int flags);

extern int my_prop_iter(my_list_t *list, my_prop_iter_fn_t func, void *user);

#endif /* __MY_UTIL_PROP_H */
