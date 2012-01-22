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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file opcode_params.h
 * Definitions for effect opcode parameters
 * @author The GemRB Project
 */

#ifndef IE_OPCODE_PARAMS_H
#define IE_OPCODE_PARAMS_H

//regen/poison/disease types
#define RPD_PERCENT 1
#define RPD_POINTS  2
#define RPD_SECONDS 3
//only poison
#define RPD_ROUNDS  4
#define RPD_TURNS   5
//only disease
#define RPD_STR 4
#define RPD_DEX 5
#define RPD_CON 6
#define RPD_INT 7
#define RPD_WIS 8
#define RPD_CHA 9
#define RPD_SLOW 10
//HoW specific disease types
#define RPD_MOLD 11
#define RPD_MOLD2 12 
//iwd2 specific disease types
#define RPD_CONTAGION 13
#define RPD_PEST 14
#define RPD_DOLOR 15

//appply spell on condition
#define COND_GOTHIT 0
#define COND_NEAR 1
#define COND_HP_HALF 2
#define COND_HP_QUART 3
#define COND_HP_LOW 4
#define COND_HELPLESS 5
#define COND_POISONED 6
#define COND_ATTACKED 7
#define COND_NEAR4 8
#define COND_NEAR10 9
#define COND_EVERYROUND 10
#define COND_TOOKDAMAGE 11

//resources for the seven eyes effect
#define EYE_MIND   0
#define EYE_SWORD  1
#define EYE_MAGE   2
#define EYE_VENOM  3
#define EYE_SPIRIT 4
#define EYE_FORT   5
#define EYE_STONE  6

//spell states
#define SS_HOPELESSNESS 0
#define SS_PROTFROMEVIL 1
#define SS_ARMOROFFAITH 2
#define SS_NAUSEA       3
#define SS_ENFEEBLED    4
#define SS_FIRESHIELD   5
#define SS_ICESHIELD    6
#define SS_HELD         7
#define SS_DEATHWARD    8
#define SS_HOLYPOWER    9
#define SS_GOODCHANT    10
#define SS_BADCHANT     11
#define SS_GOODPRAYER   12
#define SS_BADPRAYER    13
#define SS_GOODRECIT    14
#define SS_BADRECIT     15
#define SS_RIGHTEOUS    16    //allied
#define SS_RIGHTEOUS2   17    //allied and same alignment
#define SS_STONESKIN    18
#define SS_IRONSKIN     19
#define SS_SANCTUARY    20
#define SS_RESILIENT    21
#define SS_BLESS        22
#define SS_AID          23
#define SS_BARKSKIN     24
#define SS_HOLYMIGHT    25
#define SS_ENTANGLE     26
#define SS_WEB          27
#define SS_GREASE       28
#define SS_FREEACTION   29
#define SS_ENTROPY      30
#define SS_STORMSHELL   31
#define SS_ELEMPROT     32
#define SS_BERSERK      33
#define SS_BLOODRAGE    34
#define SS_NOHPINFO     35
#define SS_NOAWAKE      36
#define SS_AWAKE        37
#define SS_DEAF         38
#define SS_ANIMALRAGE   39
#define SS_NOBACKSTAB   40
#define SS_CHAOTICCMD   41
#define SS_MISCAST      42
#define SS_PAIN         43
#define SS_MALISON      44
//#define SS_CATSGRACE    45   //used explicitly
#define SS_MOLDTOUCH    46
#define SS_FLAMESHROUD  47
#define SS_EYEMIND      48
#define SS_EYESWORD     49
#define SS_EYEMAGE      50
#define SS_EYEVENOM     51
#define SS_EYESPIRIT    52
#define SS_EYEFORTITUDE 53
#define SS_EYESTONE     54
#define SS_AEGIS        55
#define SS_EXECUTIONER  56
#define SS_ENERGYDRAIN  57
#define SS_TORTOISE     58
#define SS_BLINK        59
#define SS_MINORGLOBE   60
#define SS_PROTFROMMISS 61
#define SS_GHOSTARMOR   62
#define SS_REFLECTION   63
#define SS_KAI          64
#define SS_CALLEDSHOT   65
#define SS_MIRRORIMAGE  66
#define SS_TURNED       67
#define SS_BLADEBARRIER 68
#define SS_POISONWEAPON 69
#define SS_STUNNINGBLOW 70
#define SS_QUIVERPALM   71
#define SS_DOMINATION   72
#define SS_MAJORGLOBE   73
#define SS_SHIELD       74
#define SS_ANTIMAGIC    75
#define SS_POWERATTACK  76
//more powerattack
#define SS_EXPERTISE    81
//more expertise
#define SS_ARTERIAL     86
#define SS_HAMSTRING    87
#define SS_RAPIDSHOT    88
#define SS_IRONBODY     89
#define SS_TENSER       90
#define SS_SMITEEVIL    91
#define SS_ALICORNLANCE 92
#define SS_LIGHTNING    93
#define SS_CHAMPIONS    94
#define SS_BONECIRCLE   95
#define SS_CLOAKOFFEAR  96
#define SS_PESTILENCE   97
#define SS_CONTAGION    98
#define SS_BANE         99
#define SS_DEFENSIVE    100
#define SS_DESTRUCTION  101
#define SS_DOLOROUS     102
#define SS_DOOM         103
#define SS_EXALTATION   104
#define SS_FAERIEFIRE   105
#define SS_FINDTRAPS    106
#define SS_GREATERLATH  107
#define SS_MAGICRESIST  108
#define SS_NPROTECTION  109
#define SS_PROTFROMFIRE 110
#define SS_PROTFROMLIGHTNING 111
#define SS_ELEMENTAL    112
#define SS_LATHANDER    113
#define SS_SLOWPOISON   114
#define SS_SPELLSHIELD  115
#define SS_STATICCHARGE 116
#define SS_ACIDARROW    117
#define SS_FREEZING     118
#define SS_PROTFROMACID 119
#define SS_PROTFROMELEC 120
#define SS_PFNMISSILES  121
#define SS_PROTFROMPETR 122
#define SS_ENFEEBLEMENT 123
#define SS_SEVENEYES    124
//#define SS_SOULEATER    125
#define SS_LOWERRESIST  140
#define SS_LUCK         141
#define SS_VOCALIZE     157
//tested for this, splstate is wrong or this entry has two uses
#define SS_DAYBLINDNESS 178
#define SS_REBUKED      179

#endif //IE_OPCODE_PARAMS_H
