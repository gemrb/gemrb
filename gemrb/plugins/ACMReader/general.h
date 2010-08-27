/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef _ACM_LAB_GENERAL_H
#define _ACM_LAB_GENERAL_H

// Interplay ACM signature
#define IP_ACM_SIG 0x01032897

struct ACM_Header {
	unsigned int signature;
	unsigned int samples;
	unsigned short channels;
	unsigned short rate;
	unsigned short levels : 4;
	unsigned short subblocks : 12;
};

#endif
