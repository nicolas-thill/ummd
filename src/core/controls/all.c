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

#include <stdlib.h>

#include "core/controls.h"

#include "util/list.h"
#include "util/log.h"

#define MY_CONTROL_REGISTER(c,x) { \
	extern my_control_impl_t my_control_##x; \
	my_control_register((c), &my_control_##x); \
}

void my_control_register(my_core_t *core, my_control_impl_t *control)
{
	my_list_queue(core->controls, control);
}

void my_control_register_all(my_core_t *core)
{
	MY_CONTROL_REGISTER(core, fifo);
	MY_CONTROL_REGISTER(core, osc);
/*
	MY_CONTROL_REGISTER(core, http);
	MY_CONTROL_REGISTER(core, sock);
*/
}

#ifdef MY_DEBUGGING

static int my_control_dump(void *data, void *user, int flags)
{
	my_control_impl_t *control = (my_control_impl_t *)data;

	MY_DEBUG("\t{");
	MY_DEBUG("\t\tname=\"%s\";", control->name);
	MY_DEBUG("\t\tdescription=\"%s\";", control->desc);
	MY_DEBUG("\t}%s", flags & MY_LIST_ITER_FLAG_LAST ? "" : ",");

	return 0;
}


void my_control_dump_all(my_core_t *core)
{
	MY_DEBUG("# registered control interfaces");
	MY_DEBUG("controls = (");
	my_list_iter(core->controls, my_control_dump, NULL);
	MY_DEBUG(");");

}

#endif /* MY_DEBUGGING */
