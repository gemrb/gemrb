// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file opcode_params.h
 * Definitions for effect opcode parameters
 * @author The GemRB Project
 */

#ifndef IE_OPCODE_PARAMS_H
#define IE_OPCODE_PARAMS_H

namespace GemRB {

//regen/poison/disease types
#define RPD_PERCENT 1
#define RPD_POINTS  2
#define RPD_SECONDS 3
//only poison
#define RPD_ROUNDS 4
#define RPD_TURNS  5
//iwd2 specific poison types
#define RPD_SNAKE   6
#define RPD_7       7
#define RPD_ENVENOM 8
//only disease
#define RPD_STR  4
#define RPD_DEX  5
#define RPD_CON  6
#define RPD_INT  7
#define RPD_WIS  8
#define RPD_CHA  9
#define RPD_SLOW 10
//HoW specific disease types
#define RPD_MOLD  11
#define RPD_MOLD2 12
//iwd2 specific disease types
#define RPD_CONTAGION 13
#define RPD_PEST      14
#define RPD_DOLOR     15

//appply spell on condition
#define COND_GOTHIT        0
#define COND_NEAR          1
#define COND_HP_HALF       2
#define COND_HP_QUART      3
#define COND_HP_LOW        4
#define COND_HELPLESS      5
#define COND_POISONED      6
#define COND_ATTACKED      7
#define COND_NEAR4         8
#define COND_NEAR10        9
#define COND_EVERYROUND    10
#define COND_TOOKDAMAGE    11
#define COND_KILLER        12
#define COND_TIMEOFDAY     13
#define COND_NEARX         14
#define COND_STATECHECK    15
#define COND_DIED_ME       16
#define COND_DIED_ANY      17
#define COND_TURNEDBY      18
#define COND_HP_LT         19
#define COND_HP_PERCENT_LT 20
#define COND_SPELLSTATE    21

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
#define SS_RIGHTEOUS    16 //allied
#define SS_RIGHTEOUS2   17 //allied and same alignment
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
#define SS_EXPERTISE 81
//more expertise
#define SS_ARTERIAL          86
#define SS_HAMSTRING         87
#define SS_RAPIDSHOT         88
#define SS_IRONBODY          89
#define SS_TENSER            90
#define SS_SMITEEVIL         91
#define SS_ALICORNLANCE      92
#define SS_LIGHTNING         93
#define SS_CHAMPIONS         94
#define SS_BONECIRCLE        95
#define SS_CLOAKOFFEAR       96
#define SS_PESTILENCE        97
#define SS_CONTAGION         98
#define SS_BANE              99
#define SS_DEFENSIVE         100
#define SS_DESTRUCTION       101
#define SS_DOLOROUS          102
#define SS_DOOM              103
#define SS_EXALTATION        104
#define SS_FAERIEFIRE        105
#define SS_FINDTRAPS         106
#define SS_GREATERLATH       107
#define SS_MAGICRESIST       108
#define SS_NPROTECTION       109
#define SS_PROTFROMFIRE      110
#define SS_PROTFROMLIGHTNING 111
#define SS_ELEMENTAL         112
#define SS_LATHANDER         113
#define SS_SLOWPOISON        114
#define SS_SPELLSHIELD       115
#define SS_STATICCHARGE      116
#define SS_ACIDARROW         117
#define SS_FREEZING          118
#define SS_PROTFROMACID      119
#define SS_PROTFROMELEC      120
#define SS_PFNMISSILES       121
#define SS_PROTFROMPETR      122
#define SS_ENFEEBLEMENT      123
#define SS_SEVENEYES         124
//#define SS_SOULEATER    125
#define SS_LOWERRESIST   140
#define SS_LUCK          141
#define SS_VOCALIZE      157
#define SS_BARBARIANRAGE 159
#define SS_GREATERRAGE   160
//tested for this, splstate is wrong or this entry has two uses
#define SS_DAYBLINDNESS 178
#define SS_REBUKED      179

#define SS_PRONE    249 // prevented the engine from moving a creature with STATE_SLEEPING to the backlist
#define SS_DISEASED 250

// Portrait/state icons
#define PI_RIGID      2
#define PI_CONFUSED   3
#define PI_BERSERK    4
#define PI_DRUNK      5
#define PI_POISONED   6
#define PI_DISEASED   7
#define PI_BLIND      8
#define PI_PROTFROMEVIL 9 // iwd
#define PI_HELD       13
#define PI_SLEEP      14
#define PI_BLESS      17
#define PI_FREEACTION   19 // iwd
#define PI_BARKSKIN     20 // iwd
#define PI_BANE         35 // iwd
#define PI_PANIC      36
#define PI_HASTED     38
#define PI_FATIGUE    39
#define PI_SLOWED     41
#define PI_NAUSEA       43 // iwd
#define PI_HOPELESS   44
#define PI_STUN_IWD   44 // iwd1 + iwd2
#define PI_STONESKIN_IWD    46
#define PI_LEVELDRAIN 53
#define PI_FEEBLEMIND 54
#define PI_STUN       55 // bg1 + bg2
#define PI_TENSER     55 // iwd
#define PI_AID        57
#define PI_HOLY       59
#define PI_BOUNCE     65
#define PI_BOUNCE2    67
#define PI_RIGHTEOUS    67 // iwd
#define PI_PETRIFIED  71
#define PI_CONTINGENCY   75
#define PI_BLOODRAGE     76 // iwd2
#define PI_ELEMENTS     76 // iwd
#define PI_PROJIMAGE     77
#define PI_MAZE          78
#define PI_PRISON        79
#define PI_STONESKIN     80
#define PI_DEAFNESS      83 // iwd2
#define PI_FAITHARMOR   84 // iwd
#define PI_BLEEDING     85 // iwd
#define PI_HOLYPOWER    86 // iwd
#define PI_DEATHWARD    87 // iwd
#define PI_UNCONSCIOUS  88 // iwd
#define PI_IRONSKIN     89 // iwd
#define PI_ENFEEBLEMENT 90 // iwd
#define PI_SEQUENCER     92
#define PI_ELEMPROT     93 // iwd
#define PI_MINORGLOBE   96 // iwd
#define PI_MAJORGLOBE   97 // iwd
#define PI_SHROUD       98 // iwd
#define PI_ANTIMAGIC    99 // iwd
#define PI_RESILIENT    100 // iwd
#define PI_MINDFLAYER   101 // iwd
#define PI_CLOAKOFFEAR  102 // iwd
#define PI_ENTROPY      103 // iwd
#define PI_INSECT       104 // iwd
#define PI_STORMSHELL   105 // iwd
// PI_LOWERRESIST  106 // this is different in iwd2 and bg2, but handled by data
#define PI_BLUR          109
#define PI_IMPROVEDHASTE 110
#define PI_SPELLTRAP     117
#define PI_AEGIS       119 // iwd
#define PI_EXECUTIONER 120 // iwd
#define PI_FIRESHIELD  121 // iwd
#define PI_ICESHIELD   122 // iwd
#define PI_TORTOISE 125 // iwd
#define PI_BLINK 130 // iwd
#define PI_DAYBLINDNESS 137 // iwd
#define PI_HEROIC       138 // iwd
#define PI_EMPTYBODY 145 // iwd
#define PI_CSHIELD       162
#define PI_CSHIELD2      163

}

#endif //IE_OPCODE_PARAMS_H
