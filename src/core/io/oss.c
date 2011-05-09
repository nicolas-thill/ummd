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
#include <linux/soundcard.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core/ports.h"

#include "util/log.h"
#include "util/mem.h"
#include "util/prop.h"

typedef struct my_target_priv_s my_target_priv_t;

struct my_target_priv_s {
	my_dport_t _inherited;
	char *path;
	int channels;
	int rate;
	int fd;
};

#define MY_TARGET(p) ((my_target_priv_t *)(p))
#define MY_TARGET_SIZE (sizeof(my_target_priv_t))

static int my_target_oss_event_handler(int fd, void *p)
{
	my_target_priv_t *target = MY_TARGET(p);

	/* do something */

	return 0;
}

static my_port_t *my_target_oss_create(my_core_t *core, my_port_conf_t *conf)
{
	my_port_t *port;
	char *prop;

	port = my_port_create_priv(MY_TARGET_SIZE);
	if (!port) {
		goto _MY_ERR_create_target;
	}

	prop = my_prop_lookup(conf->properties, "path");
	if (!prop) {
		my_log(MY_LOG_ERROR, "core/%s: missing 'path' property", port->conf->name);
		goto _MY_ERR_conf;
	}
	MY_TARGET(port)->path = prop;

	prop = my_prop_lookup(conf->properties, "channels");
	if (prop) {
		MY_TARGET(port)->channels = atoi(prop);
	} else {
		MY_TARGET(port)->channels = -1;
	}

	prop = my_prop_lookup(conf->properties, "rate");
	if (prop) {
		MY_TARGET(port)->rate = atoi(prop);
	} else {
		MY_TARGET(port)->rate = -1;
	}

	return port;

_MY_ERR_conf:
	my_port_destroy_priv(port);
_MY_ERR_create_target:
	return NULL;
}

static void my_target_oss_destroy(my_port_t *port)
{
	my_port_destroy_priv(port);
}

static int my_target_oss_open(my_port_t *port)
{
	int val;
	int rc;

	MY_DEBUG("core/%s: opening device '%s'", port->conf->name, MY_TARGET(port)->path);
	MY_TARGET(port)->fd = open(MY_TARGET(port)->path, O_WRONLY, 0);
	if (MY_TARGET(port)->fd == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error opening device '%s' (%d: %s)", port->conf->name, MY_TARGET(port)->path, errno, strerror(errno));
		goto _MY_ERR_open_file;
	}

	if (MY_TARGET(port)->channels != -1) {
		val = MY_TARGET(port)->channels;
		rc = ioctl(MY_TARGET(port)->fd, SNDCTL_DSP_CHANNELS, &val);
		if (rc == -1) {
			my_log(MY_LOG_ERROR, "core/%s: error setting channels for device '%s' (%d: %s)", port->conf->name, MY_TARGET(port)->path, errno, strerror(errno));
			goto _MY_ERR_ioctl_SNDCTL_DSP_CHANNELS;
		}
		MY_TARGET(port)->channels = val;
		MY_DEBUG("core/%s: device '%s', channels=%d", port->conf->name, MY_TARGET(port)->path, val);
	}

	if (MY_TARGET(port)->rate != -1) {
		val = MY_TARGET(port)->rate;
		rc = ioctl(MY_TARGET(port)->fd, SNDCTL_DSP_SPEED, &val);
		if (rc == -1) {
			my_log(MY_LOG_ERROR, "core/%s: error setting sampling rate for device '%s' (%d: %s)", port->conf->name, MY_TARGET(port)->path, errno, strerror(errno));
			goto _MY_ERR_ioctl_SNDCTL_DSP_SPEED;
		}
		MY_TARGET(port)->rate = val;
		MY_DEBUG("core/%s: device '%s', rate=%d", port->conf->name, MY_TARGET(port)->path, val);
	}

	val = AFMT_S16_NE;
	rc = ioctl(MY_TARGET(port)->fd, SNDCTL_DSP_SETFMT, &val);
	if (rc == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error setting audio format for device '%s' (%d: %s)", port->conf->name, MY_TARGET(port)->path, errno, strerror(errno));
		goto _MY_ERR_ioctl_SNDCTL_DSP_SETFMT;
	}

	my_core_event_handler_add(port->core, MY_TARGET(port)->fd, my_target_oss_event_handler, port);

	MY_DEBUG("core/%s: device '%s' opened", port->conf->name, MY_TARGET(port)->path);

	return 0;

_MY_ERR_ioctl_SNDCTL_DSP_SETFMT:
_MY_ERR_ioctl_SNDCTL_DSP_SPEED:
_MY_ERR_ioctl_SNDCTL_DSP_CHANNELS:
	close(MY_TARGET(port)->fd);
_MY_ERR_open_file:
	return -1;
}

static int my_target_oss_close(my_port_t *port)
{
	my_core_event_handler_del(port->core, MY_TARGET(port)->fd);

	MY_DEBUG("core/%s: closing device '%s'", port->conf->name, MY_TARGET(port)->path);
	if (close(MY_TARGET(port)->fd) == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error closing device '%s' (%d: %s)", port->conf->name, MY_TARGET(port)->path, errno, strerror(errno));
	}

	return 0;
}

static int my_target_oss_put(my_port_t *port, void *buf, int len)
{
	int n;

	n = write(MY_TARGET(port)->fd, buf, len);
	if (n == -1) {
		my_log(MY_LOG_ERROR, "core/%s: error writing to device '%s' (%d: %s)", port->conf->name, MY_TARGET(port)->path, errno, strerror(errno));
		return -1;
	}

	MY_DEBUG("core/%s: wrote %d bytes to device '%s'", port->conf->name, n, MY_TARGET(port)->path);

	return n;
}

my_port_impl_t my_target_oss = {
	.name = "oss",
	.desc = "Open Sound System device",
	.create = my_target_oss_create,
	.destroy = my_target_oss_destroy,
	.open = my_target_oss_open,
	.close = my_target_oss_close,
	.put = my_target_oss_put,
};
