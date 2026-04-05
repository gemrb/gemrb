# SPDX-FileCopyrightText: 2006 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# ie_slots.py - definitions of slottypes

# !!! NOTE: Keep this file synchronized with gemrb/plugins/Core/Inventory.h !!!

SLOT_HELM      = 1
SLOT_ARMOUR    = 2
SLOT_SHIELD    = 4
SLOT_GLOVE     = 8
SLOT_RING      = 16
SLOT_AMULET    = 32
SLOT_BELT      = 64
SLOT_BOOT      = 128
SLOT_WEAPON    = 256
SLOT_QUIVER    = 512
SLOT_CLOAK     = 1024
SLOT_ITEM      = 2048  #quick item
SLOT_SCROLL    = 4096
SLOT_BAG       = 8192
SLOT_POTION    = 16384
SLOT_INVENTORY = 32768
SLOT_ANY       = 32767 #any except inventory
SLOT_ALL       = 65535 #all including inventory

TYPE_NORMAL    = 0     #inventory
TYPE_ARMOR     = 1     #normal armor
TYPE_FIST      = 2     #fist weapon
TYPE_MAGIC     = 3     #magic weapon
TYPE_WEAPON    = 4     #normal weapon
TYPE_QUIVER    = 5     #projectile slots
TYPE_OFFHAND   = 6     #offhand (shield/weapon)
TYPE_HELMET    = 7     #critical hit protection
# End of file ie_slots.py
