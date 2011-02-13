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
#include "util/url.h"

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

static my_port_t *my_target_oss_create(my_port_conf_t *conf)
{
	my_port_t *target;
	char *prop;
	char url_prot[5];
	char url_path[255];

	target = my_port_priv_create(conf, MY_TARGET_SIZE);
	if (!target) {
		goto _MY_ERR_create_target;
	}

	prop = my_prop_lookup(conf->properties, "url");
	if (!prop) {
		my_log(MY_LOG_ERROR, "core/target: missing 'url' property");
		goto _MY_ERR_parse_url;
	}
	my_url_split(
		url_prot, sizeof(url_prot),
		NULL, 0, /* auth */
		NULL, 0, /* hostname */
		NULL, /* port */
		url_path, sizeof(url_path),
		prop
	);
	if (strlen(url_path) == 0) {
		my_log(MY_LOG_ERROR, "core/target: missing path component in '%s'", prop);
		goto _MY_ERR_parse_url;
	}
	MY_TARGET(target)->path = strdup(url_path);

	prop = my_prop_lookup(conf->properties, "channels");
	if (prop) {
		MY_TARGET(target)->channels = atoi(prop);
	} else {
		MY_TARGET(target)->channels = -1;
	}

	prop = my_prop_lookup(conf->properties, "rate");
	if (prop) {
		MY_TARGET(target)->rate = atoi(prop);
	} else {
		MY_TARGET(target)->rate = -1;
	}

	return target;

	free(MY_TARGET(target)->path);
_MY_ERR_parse_url:
	my_port_priv_destroy(target);
_MY_ERR_create_target:
	return NULL;
}

static void my_target_oss_destroy(my_port_t *target)
{
	free(MY_TARGET(target)->path);
	my_port_priv_destroy(target);
}

static int my_target_oss_open(my_port_t *target)
{
	int val;
	int rc;

	MY_DEBUG("target/oss: opening device '%s'", MY_TARGET(target)->path);
	MY_TARGET(target)->fd = open(MY_TARGET(target)->path, O_WRONLY, 0);
	if (MY_TARGET(target)->fd == -1) {
		my_log(MY_LOG_ERROR, "target/oss: error opening device '%s' (%s)", MY_TARGET(target)->path, strerror(errno));
		goto _MY_ERR_open_file;
	}

	if (MY_TARGET(target)->channels != -1) {
		val = MY_TARGET(target)->channels;
		rc = ioctl(MY_TARGET(target)->fd, SNDCTL_DSP_CHANNELS, &val);
		if (rc == -1) {
			my_log(MY_LOG_ERROR, "target/oss: error setting channels for device '%s' (%s)", MY_TARGET(target)->path, strerror(errno));
			goto _ERR_ioctl_SNDCTL_DSP_CHANNELS;
		}
		MY_TARGET(target)->channels = val;
		MY_DEBUG("target/oss: device '%s', channels=%d", MY_TARGET(target)->path, val);
	}

	if (MY_TARGET(target)->rate != -1) {
		val = MY_TARGET(target)->rate;
		rc = ioctl(MY_TARGET(target)->fd, SNDCTL_DSP_SPEED, &val);
		if (rc == -1) {
			my_log(MY_LOG_ERROR, "oss: error setting sampling rate for device '%s' (%s)", MY_TARGET(target)->path, strerror(errno));
			goto _ERR_ioctl_SNDCTL_DSP_SPEED;
		}
		MY_TARGET(target)->rate = val;
		MY_DEBUG("target/oss: device '%s', rate=%d", MY_TARGET(target)->path, val);
	}

	my_core_event_handler_add(target->core, MY_TARGET(target)->fd, my_target_oss_event_handler, target);

	MY_DEBUG("core/target: device '%s' opened", MY_TARGET(target)->path);

	return 0;

_ERR_ioctl_SNDCTL_DSP_SPEED:
_ERR_ioctl_SNDCTL_DSP_CHANNELS:
	close(MY_TARGET(target)->fd);
_MY_ERR_open_file:
	return -1;
}

static int my_target_oss_close(my_port_t *target)
{
	my_core_event_handler_del(target->core, MY_TARGET(target)->fd);

	MY_DEBUG("core/target: closing device '%s'", MY_TARGET(target)->path);
	if (close(MY_TARGET(target)->fd) == -1) {
		my_log(MY_LOG_ERROR, "core/target: error closing device '%s' (%s)", MY_TARGET(target)->path, strerror(errno));
	}

	return 0;
}

static int my_target_oss_put(my_port_t *port, void *buf, int len)
{
	int n;

	n = write(MY_TARGET(port)->fd, buf, len);
	if (n == -1) {
		my_log(MY_LOG_ERROR, "core/target: error writing to device '%s' (%d: %s)", MY_TARGET(port)->path, errno, strerror(errno));
		return -1;
	}

	MY_DEBUG("core/target: wrote %d bytes to '%s'", n, MY_TARGET(port)->path);

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
