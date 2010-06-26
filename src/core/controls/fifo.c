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

#include "core/controls.h"

#include "util/list.h"
#include "util/log.h"
#include "util/mem.h"

typedef struct my_control_priv my_control_priv_t;

struct my_control_priv {
	my_control_t base;
	char *path;
	int fd;
};

my_control_t *my_control_fifo_create(my_control_conf_t *conf)
{
	my_control_priv_t *control_priv;
	char *p;
	int n;

	control_priv = my_mem_alloc(sizeof(my_control_priv_t));
	if (!control_priv) {
		return NULL;
	}

	p = strchr(conf->url, ':');
	if (p) {
		n = p - conf->url;
		if (strncmp(conf->url, "file", n) != 0) {
			my_log(MY_LOG_FATAL, "unknown method '%.2$*1$s' in url '%$1s'", conf->url, n);
			goto _error;
		}
		p++;
	} else {
		p = conf->url;
	}

	control_priv->path = p;

	if (mkfifo(control_priv->path, 0600) == -1) {
		my_log(MY_LOG_FATAL, "error creating fifo '%s' (%s)", control_priv->path, strerror(errno));
		goto _error;
	}

	return (my_control_t *)control_priv;

_error:
	my_mem_free(control_priv);
	return NULL;
}

void my_control_fifo_destroy(my_control_t *control)
{
	my_control_priv_t *control_priv;

	if (unlink(control_priv->path) == -1) {
		my_log(MY_LOG_FATAL, "error removing fifo '%s' (%s)", control_priv->path, strerror(errno));
	}

	my_mem_free(control_priv);
}

int my_control_fifo_open(my_control_t *control)
{
	my_control_priv_t *control_priv = (my_control_priv_t *)control;

	control_priv->fd = open(control_priv->path, O_RDWR | O_NONBLOCK, 0);
	if (control_priv->fd == -1) {
		my_log(MY_LOG_FATAL, "error opening fifo '%s' (%s)", control_priv->path, strerror(errno));
		return -1;
	}

	return 0;
}

int my_control_fifo_close(my_control_t *control)
{
	my_control_priv_t *control_priv = (my_control_priv_t *)control;

	if (close(control_priv->fd) == -1) {
		my_log(MY_LOG_FATAL, "error closing fifo '%s' (%s)", control_priv->path, strerror(errno));
		return -1;
	}
	return 0;
}

my_control_impl_t my_control_fifo = {
	.id = MY_CONTROL_FIFO,
	.name = "fifo",
	.desc = "FIFO (named pipe) control interface",
	.create = my_control_fifo_create,
	.destroy = my_control_fifo_destroy,
	.open = my_control_fifo_open,
	.close = my_control_fifo_close,
};
