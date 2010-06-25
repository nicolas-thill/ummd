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

#include "core/controls.h"

#include "util/list.h"

#define MY_CORE_CONTROL_REGISTER(c,x) { \
	extern my_core_control_t my_core_control_##x; \
	my_core_control_register((c), &my_core_control_##x); \
}

void my_core_control_register(my_core_t *core, my_core_control_t *control)
{
	my_list_queue(core->controls, control);
}

void my_core_control_register_all(my_core_t *core)
{
/*
	MY_CORE_CONTROL_REGISTER(core, http);
	MY_CORE_CONTROL_REGISTER(core, osc);
	MY_CORE_CONTROL_REGISTER(core, sock);
*/
}
