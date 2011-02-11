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
	char *url;
	char url_prot[5];
	char url_path[255];

	target = my_port_priv_create(conf, MY_TARGET_SIZE);
	if (!target) {
		goto _MY_ERR_create_target;
	}

	url = my_prop_lookup(conf->properties, "url");
	if (!url) {
		my_log(MY_LOG_ERROR, "core/target: missing 'url' property");
		goto _MY_ERR_parse_url;
	}
	my_url_split(
		url_prot, sizeof(url_prot),
		NULL, 0, /* auth */
		NULL, 0, /* hostname */
		NULL, /* port */
		url_path, sizeof(url_path),
		url
	);
	if (strlen(url_path) == 0) {
		my_log(MY_LOG_ERROR, "core/target: missing path component in '%s'", url);
		goto _MY_ERR_parse_url;
	}
	MY_TARGET(target)->path = strdup(url_path);

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
	int channels, rate;
	int rc;

	MY_DEBUG("core/target: opening device '%s'", MY_TARGET(target)->path);
	MY_TARGET(target)->fd = open(MY_TARGET(target)->path, O_WRONLY, 0);
	if (MY_TARGET(target)->fd == -1) {
		my_log(MY_LOG_ERROR, "core/target: error opening device '%s' (%s)", MY_TARGET(target)->path, strerror(errno));
		goto _MY_ERR_open_file;
	}

	rc = ioctl(MY_TARGET(target)->fd, SNDCTL_DSP_CHANNELS, &channels);
	if (rc == -1) {
		my_log(MY_LOG_ERROR, "oss: error retrieving channels info for device '%s' (%s)", MY_TARGET(target)->path, strerror(errno));
		goto _ERR_ioctl_SNDCTL_DSP_CHANNELS;
	}

	rc = ioctl(MY_TARGET(target)->fd, SNDCTL_DSP_SPEED, &rate);
	if (rc == -1) {
		my_log(MY_LOG_ERROR, "oss: error retrieving rate info for device '%s' (%s)", MY_TARGET(target)->path, strerror(errno));
		goto _ERR_ioctl_SNDCTL_DSP_SPEED;
	}

	my_core_event_handler_add(target->core, MY_TARGET(target)->fd, my_target_oss_event_handler, target);

	MY_DEBUG("core/target: device '%s' opened, channels=%d, rate=%d", MY_TARGET(target)->path, channels, rate);

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
