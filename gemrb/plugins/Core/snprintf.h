/* 
 *	HT Editor
 *	snprintf.h
 *
 *	Copyright (C) 1999-2003 Sebastian Biallas (sb@biallas.net)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __SNPRINTF_H__
#define __SNPRINTF_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int ht_asprintf(char **ptr, const char *format, ...);
int ht_vasprintf(char **ptr, const char *format, va_list ap);

int ht_snprintf(char *str, size_t count, const char *fmt, ...);
int ht_vsnprintf(char *str, size_t count, const char *fmt, va_list args);

int ht_fprintf(FILE *file, const char *fmt, ...);
int ht_vfprintf(FILE *file, const char *fmt, va_list args);

int ht_printf(const char *fmt, ...);
int ht_vprintf(const char *fmt, va_list args);

#define snprintf ht_snprintf
#endif
