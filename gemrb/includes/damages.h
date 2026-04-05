// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DAMAGE_H
#define DAMAGE_H

namespace GemRB {

// damage types in effects
#define DAMAGE_CRUSHING        0
#define DAMAGE_ACID            1
#define DAMAGE_COLD            2
#define DAMAGE_ELECTRICITY     4
#define DAMAGE_FIRE            8
#define DAMAGE_PIERCING        0x10
#define DAMAGE_POISON          0x20
#define DAMAGE_MAGIC           0x40
#define DAMAGE_MISSILE         0x80
#define DAMAGE_SLASHING        0x100
#define DAMAGE_MAGICFIRE       0x200
#define DAMAGE_PIERCINGMISSILE 0x200 //iwd2
#define DAMAGE_MAGICCOLD       0x400
#define DAMAGE_CRUSHINGMISSILE 0x400 //iwd2
#define DAMAGE_STUNNING        0x800
#define DAMAGE_SOULEATER       0x1000 //iwd2
#define DAMAGE_BLEEDING        0x2000 //iwd2
#define DAMAGE_DISEASE         0x4000 //iwd2

#define DAMAGE_CHUNKING     0x8000
#define DAMAGE_DISINTEGRATE 0x10000 // internal gemrb marker

//damage levels
#define DL_CRITICAL     0
#define DL_BLOOD        1
#define DL_FIRE         4
#define DL_ELECTRICITY  7
#define DL_COLD         10
#define DL_ACID         13
#define DL_DISINTEGRATE 16

}

#endif
