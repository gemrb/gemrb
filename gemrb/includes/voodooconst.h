// SPDX-FileCopyrightText: 2015 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

// MAX_TRAVELING_DISTANCE is our choice, only used for party travel
// MAX_OPERATING_DISTANCE is a guess, see comment in GameScript

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

// visual range stuff
// these two are well understood for actors, but could be different for other scriptables
// eg. visual range is supposedly 15 (see note in DoObjectChecks)
// in PST it was dynamic, dependant on TNO's fighter level!?
// horizontal range was calculated as (14 + level) * (2 * 16)
// so at minimum 17 * 2 * 16 or the way we store it: 34 * 16
// VOODOO_VISUAL_RANGE is now set in Interface / Scriptable.h
// dialog range was only configurable in the iwds
static const int VOODOO_DIALOG_RANGE = 15;

}

#endif
