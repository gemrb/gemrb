/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 *
 *
 */

#define MAX_LINES 400
#define MAX_VALUE_LENGTH 20
#define MAX_TEXT_LENGTH 60 // maximum text length in case IDS file doesn't specify
#define MAX_LINE_LENGTH MAX_VALUE_LENGTH + MAX_TEXT_LENGTH
#define MAX_HEADER_LENGTH 20

#define HEADER_IDS 1
#define HEADER_LENGTH 2
#define HEADER_BLANK 3
#define HEADER_RECORD 4
#define HEADER_ERROR -1

// need to transfer this to a header file in the includes dir:
//#define IDS_VALUE_NOT_LOCATED -65535 // GetValue returns this if text is not found in arrays ... this needs to be a unique number that does not exist in the value[] array

// need to transfer this to a header file in the includes dir too:
//#define GEM_ENCRYPTION_KEY "\x88\xa8\x8f\xba\x8a\xd3\xb9\xf5\xed\xb1\xcf\xea\xaa\xe4\xb5\xfb\xeb\x82\xf9\x90\xca\xc9\xb5\xe7\xdc\x8e\xb7\xac\xee\xf7\xe0\xca\x8e\xea\xca\x80\xce\xc5\xad\xb7\xc4\xd0\x84\x93\xd5\xf0\xeb\xc8\xb4\x9d\xcc\xaf\xa5\x95\xba\x99\x87\xd2\x9d\xe3\x91\xba\x90\xca"
