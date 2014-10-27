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

/**
 * @file defsounds.h
 * Defines default sound numbers
 * @author The GemRB Project
 */

// these symbols should match defsound.2da
#ifndef IE_SOUNDS_H
#define IE_SOUNDS_H

namespace GemRB {

#define DS_OPEN              0
#define DS_CLOSE             1
#define DS_HOPEN             2
#define DS_HCLOSE            3
#define DS_BUTTON_PRESSED    4
#define DS_WINDOW_OPEN       5
#define DS_WINDOW_CLOSE      6
#define DS_OPEN_FAIL         7
#define DS_CLOSE_FAIL        8
#define DS_ITEM_GONE         9
#define DS_FOUNDSECRET       10
#define DS_PICKLOCK          11
#define DS_PICKFAIL          12
#define DS_DISARMED          13

#define DS_RAIN              20
#define DS_SNOW              21
#define DS_LIGHTNING1        22
#define DS_LIGHTNING2        23
#define DS_LIGHTNING3        24
#define DS_SOLD              25
#define DS_STOLEN            26
#define DS_DRUNK             27
#define DS_DONATE1           28
#define DS_DONATE2           29
#define DS_IDENTIFY          30
#define DS_GOTXP             31
#define DS_TOOLTIP           32

}

#endif //! IE_SOUNDS_H
