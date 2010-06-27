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
#include <string.h>

#include "util/mem.h"

void *my_mem_alloc(int n)
{
	void *p = malloc(n);
	
	if (p) {
		memset(p, 0, n);
	}
	
	return p;
}

void my_mem_free(void *p)
{
	free(p);
}

void my_mem_zero(void *p, int n)
{
	memset(p, 0, n);
}

void my_mem_copy(void *pt, void *ps, int n)
{
	memcpy(pt, ps, n);
}

