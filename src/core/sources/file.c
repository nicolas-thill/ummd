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
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core/ports.h"

#include "util/log.h"
#include "util/mem.h"
#include "util/prop.h"
#include "util/url.h"

typedef struct my_source_data_s my_source_data_t;

struct my_source_data_s {
	my_port_t _inherited;
	char *path;
	int fd;
};

#define MY_SOURCE_DATA(p) ((my_source_data_t *)(p))
#define MY_SOURCE_DATA_SIZE (sizeof(my_source_data_t))

static void my_source_file_event_handler(int fd, void *p)
{
	my_source_data_t *source = MY_SOURCE_DATA(p);

	/* do something */
}

static my_port_t *my_source_file_create(my_port_conf_t *conf)
{
	my_port_t *source;
	char *url;
	char url_prot[5];
	char url_path[255];

	source = my_port_priv_create(conf, MY_SOURCE_DATA_SIZE);
	if (!source) {
		goto _MY_ERR_create_source;
	}

	url = my_prop_lookup(conf->properties, "url");
	my_url_split(
		url_prot, sizeof(url_prot),
		NULL, 0, /* auth */
		NULL, 0, /* hostname */
		NULL, /* port */
		url_path, sizeof(url_path),
		url
	);
	if (strlen(url_prot) > 0) {
		if (strcmp(url_prot, "file") != 0) {
			my_log(MY_LOG_ERROR, "core/source: unknown url protocol '%s' in '%s'", url_prot, url);
			goto _MY_ERR_parse_url;
		}
	}
	if (strlen(url_path) == 0) {
		my_log(MY_LOG_ERROR, "core/source: missing path component in '%s'", url);
		goto _MY_ERR_parse_url;
	}

	MY_SOURCE_DATA(source)->path = strdup(url_path);

	return source;

	free(MY_SOURCE_DATA(source)->path);
_MY_ERR_parse_url:
	my_source_priv_destroy(source);
_MY_ERR_create_source:
	return NULL;
}

static void my_source_file_destroy(my_port_t *source)
{
	free(MY_SOURCE_DATA(source)->path);
	my_port_priv_destroy(source);
}

static int my_source_file_open(my_port_t *source)
{
	MY_DEBUG("core/source: opening file '%s'", MY_SOURCE_DATA(source)->path);
	MY_SOURCE_DATA(source)->fd = open(MY_SOURCE_DATA(source)->path, O_RDONLY | O_NONBLOCK, 0);
	if (MY_SOURCE_DATA(source)->fd == -1) {
		my_log(MY_LOG_ERROR, "core/source: error opening file '%s' (%s)", MY_SOURCE_DATA(source)->path, strerror(errno));
		goto _MY_ERR_open_file;
	}

	my_core_event_handler_add(source->core, MY_SOURCE_DATA(source)->fd, my_source_file_event_handler, source);

	return 0;

_MY_ERR_open_file:
	return -1;
}

static int my_source_file_close(my_port_t *source)
{
	my_core_event_handler_del(source->core, MY_SOURCE_DATA(source)->fd);

	MY_DEBUG("core/source: closing file '%s'", MY_SOURCE_DATA(source)->path);
	if (close(MY_SOURCE_DATA(source)->fd) == -1) {
		my_log(MY_LOG_ERROR, "core/source: error closing file '%s' (%s)", MY_SOURCE_DATA(source)->path, strerror(errno));
	}

	return 0;
}

my_port_impl_t my_source_file = {
	.name = "file",
	.desc = "Regular file source",
	.create = my_source_file_create,
	.destroy = my_source_file_destroy,
	.open = my_source_file_open,
	.close = my_source_file_close,
};
