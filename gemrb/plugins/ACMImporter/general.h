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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ACMImporter/general.h,v 1.3 2005/02/23 22:20:52 guidoj Exp $
 *
 */

#ifndef _ACM_LAB_GENERAL_H
#define _ACM_LAB_GENERAL_H

// Interplay ACM signature
#define IP_ACM_SIG 0x01032897

#pragma pack (push, 1)
struct ACM_Header {
	unsigned int signature;
	unsigned int samples;
	unsigned short channels;
	unsigned short rate;
	unsigned short levels : 4;
	unsigned short subblocks : 12;
};
#pragma pack (pop)

#endif
