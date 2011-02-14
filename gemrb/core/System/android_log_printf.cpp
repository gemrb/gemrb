/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#include <stdarg.h>
#include <android/log.h>

int android_log_printf(const char * fmt, ...) {
	int return_value;
	va_list ap;
	va_start(ap, fmt);
	int characters = vfprintf(stdout, fmt, ap); // determine buffer size
	if(characters<0) return characters;
	char* buff = new char[characters+1];
	return_value = vsprintf(buff, fmt, ap);
	va_end(ap);
	__android_log_print(ANDROID_LOG_INFO, "printf:", buff);
	delete buff;
	return return_value;
}
