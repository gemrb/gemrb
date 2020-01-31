/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2015 The GemRB Project
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
 * @file voodooconst.h
 * this file contains constants of dubious nature. Most
 * were figured out by trial and error and may be way off.
 * Or even different between the engines.
 * FIXME: THIS FILE SHOULD NOT NEED TO EXIST!
 * @author The GemRB Project
 */

#ifndef VOODOO_H
#define VOODOO_H

#include "exports.h"

namespace GemRB {

// Constant to convert from points to (feet) for spell distance calculation
// completely empirical, 9 seems to work fine for iwd and bgs
// but it is either too small for iwd2 or there are other bugs
// (fireball to the door in the targos attack is a good test case)
static const int VOODOO_SPL_RANGE_F = 15;

// ... similarly for items
static const int VOODOO_ITM_RANGE_F = 15;

// fx_casting_glow has hardcoded height offsets, while they should be avatar based
// ypos_by_direction and xpos_by_direction

// MAX_TRAVELING_DISTANCE is our choice, only used for party travel
// MAX_OPERATING_DISTANCE is a guess
// test cases: tob pp summoning spirit, pst portals, pst AR0405, AR0508, ar0500 (guards through gates)
// it's about 3 times bigger in pst, perhaps related to the bigger sprite sizes and we modify it in Scriptable
// The distance of operating a trigger, container, dialog buffer etc.
static unsigned int MAX_OPERATING_DISTANCE IGNORE_UNUSED = 40; //a search square is 16x12 (diagonal of 20), so that's about two

// existence delay is a stat used to delay various char quips, but it's sometimes set to 0,
// while it should clearly always be delayed at least a bit. The engine uses randomization.
// Estimates from bg1 research:
/*
 75 = avg.  5 s
150 = avg. 10 s
225 = avg. 15 s
300 = avg. 20 s <- BG1 default
375 = avg. 25 s
450 = avg. 30 s
525 = avg. 35 s
600 = avg. 40 s
675 = avg. 45 s
750 = avg. 50 s
825 = avg. 55 s
900 = avg. 60 s*/
static const unsigned int VOODOO_EXISTENCE_DELAY_DEFAULT = 300;

// NearLocation range multiplier (currently the same for pst and iwd2/how)
// arbitrary, started as 20 and has no effect for callers that want exact position
// supposedly the same feet->map conversion as usual
static const int VOODOO_NEARLOC_F = 15; // sqrt(8*8+12+12)

// visual range stuff
static const int VOODOO_CANSEE_F = 16;
// these two are well understood for actors, but could be different for other scriptables
// eg. visual range is supposedly 15 (see note in DoObjectChecks)
static const int VOODOO_VISUAL_RANGE = 28;
static const int VOODOO_DIALOG_RANGE = 15;

// character speed was also hardcoded depending on the used animation type
// 9 is a good default for bg2, but it's clearly wrong for bg1 and some animations
// we now use the number of frames in each cycle of its animation (bad)
static const int VOODOO_CHAR_SPEED = 9;

}

#endif
