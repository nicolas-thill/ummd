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

#include "core/filters.h"

#include "util/list.h"

#define MY_CORE_FILTER_REGISTER(c,x) { \
	extern my_core_filter_t my_core_filter_##x; \
	my_core_filter_register((c), &my_core_filter_##x); \
}

void my_core_filter_register(my_core_t *core, my_core_filter_t *filter)
{
	my_list_queue(core->filters, filter);
}

void my_core_filter_register_all(my_core_t *core)
{
/*
	MY_CORE_FILTER_REGISTER(core, delay);
*/
}
