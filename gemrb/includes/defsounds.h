// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file defsounds.h
 * Defines default sound numbers
 * @author The GemRB Project
 */

// these symbols should match defsound.2da
#ifndef IE_SOUNDS_H
#define IE_SOUNDS_H

namespace GemRB {

#define DS_OPEN           0
#define DS_CLOSE          1
#define DS_HOPEN          2
#define DS_HCLOSE         3
#define DS_BUTTON_PRESSED 4
#define DS_WINDOW_OPEN    5
#define DS_WINDOW_CLOSE   6
#define DS_OPEN_FAIL      7
#define DS_CLOSE_FAIL     8
#define DS_ITEM_GONE      9
#define DS_FOUNDSECRET    10
#define DS_PICKLOCK       11
#define DS_PICKFAIL       12
#define DS_DISARMED       13

#define DS_RAIN       20
#define DS_SNOW       21
#define DS_LIGHTNING1 22
#define DS_LIGHTNING2 23
#define DS_LIGHTNING3 24
#define DS_SOLD       25
#define DS_STOLEN     26
#define DS_DRUNK      27
#define DS_DONATE1    28
#define DS_DONATE2    29
#define DS_IDENTIFY   30
#define DS_GOTXP      31
#define DS_TOOLTIP    32

}

#endif //! IE_SOUNDS_H
