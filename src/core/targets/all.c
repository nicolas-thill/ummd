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

#include "core/targets.h"

#include "util/list.h"

#define MY_CORE_TARGET_REGISTER(c,x) { \
	extern my_core_target_t my_core_target_##x; \
	my_core_target_register((c), &my_core_target_##x); \
}

static my_list_t my_core_targets;

void my_core_target_register(my_core_t *core, my_core_target_t *target)
{
	my_list_queue(core->targets, target);
}

void my_core_target_register_all(my_core_t *core)
{
/*
	MY_CORE_TARGET_REGISTER(core, file);
	MY_CORE_TARGET_REGISTER(core, sock);
	MY_CORE_TARGET_REGISTER(core, net_http_client);
	MY_CORE_TARGET_REGISTER(core, net_rtp_client);
	MY_CORE_TARGET_REGISTER(core, net_udp_client);
*/
}