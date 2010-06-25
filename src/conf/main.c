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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "conf.h"

#include "util/list.h"
#include "util/log.h"
#include "util/mem.h"

void my_conf_init(my_conf_t *conf)
{
	my_mem_zero(conf, sizeof(*conf));

	conf->controls = my_list_create();
	if (conf->controls == NULL) {
		MY_ERROR("error control list (%s)" , strerror(errno));
		exit(1);
	}

	conf->filters = my_list_create();
	if (conf->filters == NULL) {
		MY_ERROR("error filter list (%s)" , strerror(errno));
		exit(1);
	}

	conf->sources = my_list_create();
	if (conf->sources == NULL) {
		MY_ERROR("error source list (%s)" , strerror(errno));
		exit(1);
	}

	conf->targets = my_list_create();
	if (conf->targets == NULL) {
		MY_ERROR("error target list (%s)" , strerror(errno));
		exit(1);
	}

	conf->wirings = my_list_create();
	if (conf->wirings == NULL) {
		MY_ERROR("error wiring list (%s)" , strerror(errno));
		exit(1);
	}
}
