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
 * $Id$
 *
 */

/**
 * @file strrefs.h
 * Defines indices of "standard" strings in strings.2da files
 * @author The GemRB Project
 */


// these symbols should match strings.2da

#ifndef IE_STRINGS_H
#define IE_STRINGS_H

#define STR_SCATTERED      0
#define STR_WHOLEPARTY     1
#define STR_DOORLOCKED     2
#define STR_MAGICTRAP      3
#define STR_NORMALTRAP     4
#define STR_TRAP           5
#define STR_CANNOTGO       6
#define STR_TRAPREMOVED    7
#define STR_OVERSTOCKED    8
#define STR_SLEEP          9
#define STR_AMBUSH         10
#define STR_CONTLOCKED     11
#define STR_NOMONEY        12
#define STR_CURSED         13
#define STR_SPELLDISRUPT   14
#define STR_DIED           15
#define STR_MAYNOTREST     16
#define STR_CANTRESTMONS   17
#define STR_CANTSAVEMONS   18
#define STR_CANTSAVE       19
#define STR_NODIALOG       20
#define STR_CANTSAVEDIALOG 21
#define STR_CANTSAVEDIALOG2 22
#define STR_CANTSAVEMOVIE   23
#define STR_TARGETBUSY      24
#define STR_CANTTALKTRANS   25
#define STR_GOTGOLD         26
#define STR_LOSTGOLD        27
#define STR_GOTXP           28
#define STR_LOSTXP          29
#define STR_GOTITEM         30
#define STR_LOSTITEM        31
#define STR_GOTREP          32
#define STR_LOSTREP         33
#define STR_GOTABILITY      34
#define STR_GOTSPELL        35
#define STR_GOTSONG         36
#define STR_NOTHINGTOSAY    37
#define STR_JOURNALCHANGE   38
#define STR_WORLDMAPCHANGE  39
#define STR_PAUSED          40
#define STR_UNPAUSED        41
#define STR_SCRIPTPAUSED    42
#define STR_AP_UNUSABLE     43
#define STR_AP_ATTACKED     44
#define STR_AP_HIT          45
#define STR_AP_WOUNDED      46
#define STR_AP_DEAD         47
#define STR_AP_NOTARGET     48
#define STR_AP_ENDROUND     49
#define STR_AP_ENEMY        50
#define STR_AP_TRAP         51
#define STR_AP_SPELLCAST    52
#define STR_AP_RESERVED1    53
#define STR_AP_RESERVED2    54
#define STR_AP_RESERVED3    55
#define STR_AP_RESERVED4    56
#define STR_CHARMED         57
#define STR_DIRECHARMED     58
#define STR_CONTROLLED      59
#define STR_EVIL            60
#define STR_GE_NEUTRAL      61
#define STR_GOOD            62
#define STR_LAWFUL          63
#define STR_LC_NEUTRAL      64
#define STR_CHAOTIC         65
#define STR_ACTION_CAST     66
#define STR_ACTION_ATTACK   67
#define STR_ACTION_TURN     68
#define STR_ACTION_SONG     69
#define STR_ACTION_FINDTRAP 70
#define STR_MAGICWEAPON     71
#define STR_OFFHAND_USED    72
#define STR_TWOHANDED_USED  73
#define STR_CANNOT_USE_ITEM 74
#define STR_CANT_DROP_ITEM  75
#define STR_NOT_IN_OFFHAND  76
#define STR_ITEM_IS_CURSED  77
#define STR_NO_CRITICAL	    78
#define STR_TRACKING        79
#define STR_TRACKINGFAILED  80
#define STR_DOOR_NOPICK     81
#define STR_CONT_NOPICK     82
#define STR_CANTSAVECOMBAT  83
#define STR_CANTSAVENOCTRL  84
#define STR_LOCKPICK_DONE   85
#define STR_LOCKPICK_FAILED 86
#define STR_STATIC_DISS     87
#define STR_LIGHTNING_DISS  88
#define STR_UNUSABLEITEM    89       //item has no usable ability
#define STR_ITEMID          90       //item needs identify
#define STR_WRONGITEMTYPE   91
#define STR_ITEMEXCL        92
#define STR_PICKPOCKET_DONE 93       //done
#define STR_PICKPOCKET_NONE 94       //no items to steal
#define STR_PICKPOCKET_FAIL 95       //failed, noticed
#define STR_PICKPOCKET_EVIL 96       //can't pick hostiles
#define STR_PICKPOCKET_ARMOR 97      //armor restriction
#define STR_USING_FEAT      98
#define STR_STOPPED_FEAT    99
#define STR_DISARM_DONE 100       //trap disarmed
#define STR_DISARM_FAIL 101       //trap not disarmed
#define STR_DOORBASH_DONE 102
#define STR_DOORBASH_FAIL 103
#define STR_CONTBASH_DONE 104
#define STR_CONTBASH_FAIL 105
#define STR_MAYNOTSETTRAP 106
#define STR_SNAREFAILED   107
#define STR_SNARESUCCEED  108
#define STR_NOMORETRAP    109
#define STR_DISABLEDMAGE  110
#define STR_SAVESUCCEED   111
#define STR_QSAVESUCCEED  112
#define STR_UNINJURED     113       //uninjured
#define STR_INJURED1      114
#define STR_INJURED2      115
#define STR_INJURED3      116
#define STR_INJURED4      117       //near death
#define STR_HOURS         118       //<HOUR> hours
#define STR_HOUR          119
#define STR_DAYS          120       //<GAMEDAYS> days
#define STR_DAY           121
#define STR_REST          122       //You have rested for <DURATION>
#define STR_JOURNEY       123       //The journey took <DURATION>
#define STR_PST_REST      124       //You have rested for <HOUR> <DURATION>
#define STR_PST_HOUR      125
#define STR_PST_HOURS     126
#define STR_DAMAGE_IMMUNITY 127
#define STR_DAMAGE1 128
#define STR_DAMAGE2 129
#define STR_DAMAGE3 130
#define STR_DMG_POISON 131
#define STR_DMG_MAGIC 132
#define STR_DMG_MISSILE 133
#define STR_DMG_SLASHING 134
#define STR_DMG_PIERCING 135
#define STR_DMG_CRUSHING 136
#define STR_DMG_FIRE 137
#define STR_DMG_ELECTRIC 138
#define STR_DMG_COLD 139
#define STR_DMG_ACID 140
#define STR_DMG_OTHER 141
#define STR_GOTQUESTXP 142
#define STR_LEVELUP 143
#define STR_INVFULL_ITEMDROP 144
#define STR_CONTDUP 145
#define STR_CONTTRIG 146
#define STR_CONTFAIL 147
#define STR_SEQDUP 148

#define STRREF_COUNT 149

#endif //! IE_STRINGS_H
