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

#include "core/sources.h"

#include "util/list.h"
#include "util/log.h"

static my_list_t my_sources;

#define MY_SOURCE_REGISTER(x) { \
	extern my_source_impl_t my_source_##x; \
	my_source_register(&my_source_##x); \
}

static void my_source_register(my_source_impl_t *source)
{
	my_list_enqueue(&my_sources, source);
}

void my_source_register_all(void)
{
/*
	MY_SOURCE_REGISTER(core, file);
	MY_SOURCE_REGISTER(core, sock);
	MY_SOURCE_REGISTER(core, net_http_client);
	MY_SOURCE_REGISTER(core, net_rtp_client);
	MY_SOURCE_REGISTER(core, net_udp_client);
*/
}

#ifdef MY_DEBUGGING

static int my_source_dump_fn(void *data, void *user, int flags)
{
	my_source_impl_t *source = (my_source_impl_t *)data;

	MY_DEBUG("\t{");
	MY_DEBUG("\t\tname=\"%s\";", source->name);
	MY_DEBUG("\t\tdescription=\"%s\";", source->desc);
	MY_DEBUG("\t}%s", flags & MY_LIST_ITER_FLAG_LAST ? "" : ",");

	return 0;
}

void my_source_dump_all(void)
{
	MY_DEBUG("# registered sources");
	MY_DEBUG("sources = (");
	my_list_iter(&my_sources, my_source_dump_fn, NULL);
	MY_DEBUG(");");

}

#endif /* MY_DEBUGGING */
