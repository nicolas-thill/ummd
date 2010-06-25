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

#include "core.h"
#include "core/controls.h"
#include "core/filters.h"
#include "core/sources.h"
#include "core/targets.h"
#include "core/wirings.h"

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


#ifdef MY_DEBUGGING

static int my_conf_dump_control(void *data, void *user, int flags)
{
	my_control_conf_t *control = (my_control_conf_t *)data;

	MY_DEBUG("\tcontrol[%d] = {", control->index);
	MY_DEBUG("\t\tname=\"%s\";", control->name);
	MY_DEBUG("\t\tdescription=\"%s\";", control->desc);
	MY_DEBUG("\t\ttype=\"%s\";", control->type);
	MY_DEBUG("\t\turl=\"%s\";", control->url);
	MY_DEBUG("\t}%s", flags & MY_LIST_ITER_FLAG_LAST ? "" : ",");

	return 0;
}

static int my_conf_dump_filter(void *data, void *user, int flags)
{
	my_filter_conf_t *filter = (my_filter_conf_t *)data;

	MY_DEBUG("\tfilter[%d] = {", filter->index);
	MY_DEBUG("\t\tname=\"%s\";", filter->name);
	MY_DEBUG("\t\tdescription=\"%s\";", filter->desc);
	MY_DEBUG("\t\ttype=\"%s\";", filter->type);
	MY_DEBUG("\t\targ=\"%s\";", filter->arg);
	MY_DEBUG("\t}%s", flags & MY_LIST_ITER_FLAG_LAST ? "" : ",");

	return 0;
}

static int my_conf_dump_source(void *data, void *user, int flags)
{
	my_source_conf_t *source = (my_source_conf_t *)data;

	MY_DEBUG("\tsource[%d] = {", source->index);
	MY_DEBUG("\t\tname=\"%s\";", source->name);
	MY_DEBUG("\t\tdescription=\"%s\";", source->desc);
	MY_DEBUG("\t\ttype=\"%s\";", source->type);
	MY_DEBUG("\t\turl=\"%s\";", source->url);
	MY_DEBUG("\t}%s", flags & MY_LIST_ITER_FLAG_LAST ? "" : ",");

	return 0;
}

static int my_conf_dump_target(void *data, void *user, int flags)
{
	my_target_conf_t *target= (my_target_conf_t *)data;

	MY_DEBUG("\ttarget[%d] = {", target->index);
	MY_DEBUG("\t\tname=\"%s\";", target->name);
	MY_DEBUG("\t\tdescription=\"%s\";", target->desc);
	MY_DEBUG("\t\ttype=\"%s\";", target->type);
	MY_DEBUG("\t\turl=\"%s\";", target->url);
	MY_DEBUG("\t}%s", flags & MY_LIST_ITER_FLAG_LAST ? "" : ",");

	return 0;
}

static int my_conf_dump_wiring(void *data, void *user, int flags)
{
	my_wiring_conf_t *wiring = (my_wiring_conf_t *)data;

	MY_DEBUG("\twiring[%d] = {", wiring->index);
	MY_DEBUG("\t\tname=\"%s\";", wiring->name);
	MY_DEBUG("\t\tdescription=\"%s\";", wiring->desc);
	MY_DEBUG("\t\tsource=\"%s\";", wiring->source);
	MY_DEBUG("\t\ttarget=\"%s\";", wiring->target);
	MY_DEBUG("\t}%s", flags & MY_LIST_ITER_FLAG_LAST ? "" : ",");

	return 0;
}

void my_conf_dump(my_conf_t *conf)
{
	MY_DEBUG("cfg-file = \"%s\";", conf->cfg_file);
	MY_DEBUG("log-file = \"%s\";", conf->log_file);
	MY_DEBUG("log-level = %d;", conf->log_level);
	MY_DEBUG("pid-file = \"%s\";", conf->pid_file);

	MY_DEBUG("controls = (");
	my_list_iter(conf->controls, my_conf_dump_control, NULL);
	MY_DEBUG(");");

	MY_DEBUG("filters = (");
	my_list_iter(conf->filters, my_conf_dump_filter, NULL);
	MY_DEBUG(");");

	MY_DEBUG("sources = (");
	my_list_iter(conf->sources, my_conf_dump_source, NULL);
	MY_DEBUG(");");

	MY_DEBUG("targets = (");
	my_list_iter(conf->targets, my_conf_dump_target, NULL);
	MY_DEBUG(");");

	MY_DEBUG("wirings = (");
	my_list_iter(conf->wirings, my_conf_dump_wiring, NULL);
	MY_DEBUG(");");
}

#endif /* MY_DEBUGGING */
