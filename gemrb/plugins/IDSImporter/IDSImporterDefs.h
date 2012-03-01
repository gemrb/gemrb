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

namespace GemRB {

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

}
