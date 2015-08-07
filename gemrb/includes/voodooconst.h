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

namespace GemRB {

// Constant to convert from points to (feet) for spell distance calculation
// completely empirical, 9 seems to work fine for iwd and bgs
// but it is either too small for iwd2 or there are other bugs
// (fireball to the door in the targos attack is a good test case)
static const int VOODOO_SPL_RANGE_F = 9;

// factors for our guess for proper weapon ranges
// long bows and xbows have a range of 100, shortbows 75, while melee weapons around 0
// 400 units is about the normal sight range
static const int VOODOO_WPN_RANGE1 = 10; // melee
static const int VOODOO_WPN_RANGE2 = 4;  // ranged weapons

// a multiplier for visual range that we use in the trap finding modal action/effect
static const int VOODOO_FINDTRAP_RANGE = 10;

// fx_casting_glow has hardcoded height offsets, while they should be avatar based
// ypos_by_direction and xpos_by_direction

// MAX_TRAVELING_DISTANCE is our choice, only used for party travel
// MAX_OPERATING_DISTANCE is a guess too and seems to be wrong for pst
// FIXME: not included here until more is known, since it is used in many files
// test cases: tob pp summoning spirit, pst portals, pst AR0405 and AR0508

// used for the shout action, supposedly "slightly larger than the default visual radius of NPCs"
// while it looks too big, it still seems too small for pst
static const int VOODOO_SHOUT_RANGE = 400;

// visual range stuff
static const int VOODOO_CANSEE_F = 15;
// these two are well understood for actors, but could be different for other scriptables
// eg. visual range is supposedly 15 (see note in DoObjectChecks)
static const int VOODOO_VISUAL_RANGE = 30;
static const int VOODOO_DIALOG_RANGE = 15;

// character speed was also hardcoded depending on the used animation type
// 9 is a good default for bg2, but it's clearly wrong for bg1 and some animations
// we now use the number of frames in each cycle of its animation (bad)
static const int VOODOO_CHAR_SPEED = 9;

}

#endif
