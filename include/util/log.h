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

#ifndef __MY_UTIL_LOG_H
#define __MY_UTIL_LOG_H

#include <stdarg.h>
 
#include "autoconf.h"

#define MY_LOG_FATAL    0
#define MY_LOG_ERROR    1
#define MY_LOG_WARNING  2
#define MY_LOG_NOTICE   3
#define MY_LOG_INFO     4
#define MY_LOG_DEBUG    5

#ifdef MY_DEBUGGING
# define MY_DEBUG(fmt, args...) my_warn(MY_LOG_DEBUG, fmt , ## args)
#else
# define MY_DEBUG(fmt, args...)
#endif

#define MY_ERROR(fmt, args...) my_warn(MY_LOG_FATAL, fmt , ## args)

extern int my_log_init(char *prefix);

extern int my_log_open(char *url, int level);
extern void my_log_close(void);

extern void my_log(int level, const char *fmt, ...);
extern void my_vlog(int level, const char *fmt, va_list va);
extern void my_warn(int level, const char *fmt, ...);

#endif /* __MY_UTIL_LOG_H */
