/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2007 The GemRB Project
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

/**
 * @file overlays.h
 * the possible hardcoded overlays (they got separate stats or bits)
 * the numbers are compliant with the internals of IWD2
 * @author The GemRB Project
 */

namespace GemRB {

#define OVERLAY_COUNT  32

#define OV_SANCTUARY   0
#define OV_ENTANGLE    1
#define OV_WISP        2  //iwd2
#define OV_SPELLTRAP   2  //bg2
#define OV_SHIELDGLOBE 3
#define OV_GREASE      4
#define OV_WEB         5
#define OV_MINORGLOBE  6
#define OV_GLOBE       7
#define OV_SHROUD      8
#define OV_ANTIMAGIC   9
#define OV_RESILIENT   10
#define OV_NORMALMISS  11
#define OV_CLOAKFEAR   12
#define OV_ENTROPY     13
#define OV_FIREAURA    14
#define OV_FROSTAURA   15
#define OV_INSECT      16
#define OV_STORMSHELL  17
#define OV_LATH1       18
#define OV_LATH2       19
#define OV_GLATH1      20
#define OV_GLATH2      21
#define OV_SEVENEYES   22
#define OV_SEVENEYES2  23
#define OV_BOUNCE      24  //bouncing
#define OV_BOUNCE2     25  //bouncing activated
#define OV_FIRESHIELD1 26
#define OV_FIRESHIELD2 27
#define OV_ICESHIELD1  28
#define OV_ICESHIELD2  29
#define OV_TORTOISE    30
#define OV_DEATHARMOR  31

}
