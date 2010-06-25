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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "util/log.h"

#include "util/mem.h"

#define MY_LOG_BUF_SIZE  1024

typedef int (*my_log_open_func_t)(void);
typedef void (*my_log_put_func_t)(int level, const char *fmt, va_list va);
typedef void (*my_log_close_func_t)(void);

typedef struct {
	int level;
	char *prefix;
	char *path;
	FILE *stream;
	my_log_open_func_t open_func;
	my_log_put_func_t put_func;
	my_log_close_func_t close_func;
} my_log_data_t;

static my_log_data_t log_data;

static int my_log_file_open(void);
static void my_log_file_put(int level, const char *fmt, va_list va);
static void my_log_file_close(void);

static int my_log_syslog_open(void);
static void my_log_syslog_put(int level, const char *fmt, va_list va);
static void my_log_syslog_close(void);

static void my_log_fprintf(FILE *stream, int level, const char *fmt, va_list va);
static void my_log_vsyslog(int level, const char *fmt, va_list va);

int my_log_init(char *prefix)
{
	my_mem_zero(&log_data, sizeof(log_data));
	log_data.prefix = prefix;
}

int my_log_open(char *url, int level)
{
	char *p;
	int n;

	log_data.level = level;

	p = strchr(url, ':');
	if (p) {
		n = p - url;
		p++;
	} else {
		n = strlen(url);
	}
	
	if (strncmp(url, "stderr", n) == 0) {
		log_data.put_func = my_log_file_put;
		log_data.stream = stderr;
	} else if (strncmp(url, "stdout", n) == 0) {
		log_data.put_func = my_log_file_put;
		log_data.stream = stdout;
	} else if (strncmp(url, "syslog", n) == 0) {
		log_data.open_func = my_log_syslog_open;
		log_data.put_func = my_log_syslog_put;
		log_data.close_func = my_log_syslog_close;
	} else {
		if(p) {
			if (strncmp(url, "file", n) != 0) {
				MY_ERROR("unknown log method '%s'", url);
				return -1;
			}
			log_data.path = p;
		} else {
			log_data.path = url;
		}
		log_data.open_func = my_log_file_open;
		log_data.put_func = my_log_file_put;
		log_data.close_func = my_log_file_close;
	}

	if (log_data.open_func != NULL) {
		return (log_data.open_func)();
	}

	return 0;
}

void my_log_close(void)
{
	if (log_data.close_func != NULL) {
		(log_data.close_func)();
	}
}


void my_log(int level, const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);

	my_vlog(level, fmt, va);
}

void my_vlog(int level, const char *fmt, va_list va)
{
	if (level <= log_data.level) {
		(log_data.put_func)(level, fmt, va);
	}
}

void my_warn(int level, const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);

	my_log_fprintf(stderr, level, fmt, va);
}


int my_log_file_open(void)
{
	log_data.stream = fopen(log_data.path, "a");
	if (log_data.stream == NULL) {
		MY_ERROR("error opening log file '%s' (%s)", log_data.path, strerror(errno));
		return -1;
	}

	return 0;
}

void my_log_file_put(int level, const char *fmt, va_list va)
{
	my_log_fprintf(log_data.stream, level, fmt, va);
}


void my_log_file_close(void)
{
	fclose(log_data.stream);
}


int my_log_syslog_open(void)
{
	openlog(log_data.prefix, LOG_NDELAY | LOG_PID, LOG_DAEMON);

	return 0;
}

void my_log_syslog_put(int level, const char *fmt, va_list va)
{
	my_log_vsyslog(level, fmt, va);
}

void my_log_syslog_close(void)
{
	closelog();
}


void my_log_fprintf(FILE *stream, int level, const char *fmt, va_list va)
{
	char *log_level;
	char log_buf[MY_LOG_BUF_SIZE];

	switch (level) {
	case MY_LOG_FATAL:
		log_level = "fatal";
		break;
	case MY_LOG_ERROR:
		log_level = "error";
		break;
	case MY_LOG_WARNING:
		log_level = "warn";
		break;
	case MY_LOG_DEBUG:
		log_level = "debug";
		break;
	default:
		log_level = "info";
		break;
	}

	log_buf[0] = '\0';

	vsnprintf(log_buf, sizeof(log_buf), fmt, va);
	fprintf(stream, "%s: [%s] %s\n", log_data.prefix, log_level, log_buf);
}

void my_log_vsyslog(int level, const char *fmt, va_list va)
{
	int log_prio;

	switch (level) {
	case MY_LOG_FATAL:
		log_prio = LOG_CRIT;
		break;
	case MY_LOG_ERROR:
		log_prio = LOG_ERR;
		break;
	case MY_LOG_WARNING:
		log_prio = LOG_WARNING;
		break;
	case MY_LOG_NOTICE:
		log_prio = LOG_NOTICE;
		break;
	case MY_LOG_DEBUG:
		log_prio = LOG_DEBUG;
		break;
	default:
		log_prio = LOG_INFO;
		break;
	}

	vsyslog(log_prio, fmt, va);
}
