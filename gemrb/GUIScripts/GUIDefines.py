# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIDefines.py - common definitions of GUI-related constants for GUIScripts

# view flags
IE_GUI_VIEW_RESIZE_NONW			= 0
IE_GUI_VIEW_RESIZE_TOP			= 1 << 0 # keep my top relative to my super
IE_GUI_VIEW_RESIZE_BOTTOM		= 1 << 1 # keep my bottom relative to my super
IE_GUI_VIEW_RESIZE_VERTICAL		= IE_GUI_VIEW_RESIZE_TOP|IE_GUI_VIEW_RESIZE_BOTTOM # top+bottom effectively resizes me vertically
IE_GUI_VIEW_RESIZE_LEFT			= 1 << 3 # keep my left relative to my super
IE_GUI_VIEW_RESIZE_RIGHT		= 1 << 4 # keep my right relative to my super
IE_GUI_VIEW_RESIZE_HORIZONTAL	= IE_GUI_VIEW_RESIZE_LEFT | IE_GUI_VIEW_RESIZE_RIGHT # left + right effectively resizes me horizontally
IE_GUI_VIEW_RESIZE_ALL			= IE_GUI_VIEW_RESIZE_VERTICAL|IE_GUI_VIEW_RESIZE_HORIZONTAL # resize me relative to my super

# TODO: move these to TextContainer
IE_GUI_VIEW_RESIZE_WIDTH	= 1 << 27	# resize the view horizontally if horizontal content exceeds width
IE_GUI_VIEW_RESIZE_HEIGHT	= 1 << 26	# resize the view vertically if vertical content exceeds width

IE_GUI_VIEW_INVISIBLE		= 1 << 30
IE_GUI_VIEW_DISABLED		= 1 << 29
IE_GUI_VIEW_IGNORE_EVENTS	= 1 << 28

#button flags
IE_GUI_BUTTON_NORMAL     = 0x00000004   #default button, doesn't stick
IE_GUI_BUTTON_NO_IMAGE   = 0x00000001
IE_GUI_BUTTON_PICTURE    = 0x00000002
IE_GUI_BUTTON_SOUND      = 0x00000004
IE_GUI_BUTTON_CAPS       = 0x00000008   #capitalize all the text
IE_GUI_BUTTON_CHECKBOX   = 0x00000010   #or radio button
IE_GUI_BUTTON_RADIOBUTTON= 0x00000020   #sticks in a state

#these bits are hardcoded in the .chu structure, don't move them
IE_GUI_BUTTON_ALIGN_LEFT = 0x00000100
IE_GUI_BUTTON_ALIGN_RIGHT= 0x00000200
IE_GUI_BUTTON_ALIGN_TOP  = 0x00000400
IE_GUI_BUTTON_ALIGN_BOTTOM = 0x00000800
IE_GUI_BUTTON_ALIGN_ANCHOR = 0x00001000
IE_GUI_BUTTON_LOWERCASE    = 0x00002000
#IE_GUI_BUTTON_MULTILINE    = 0x00004000 # don't set the SINGLE_LINE font rendering flag
#end of hardcoded section

IE_GUI_BUTTON_NO_TEXT    = 0x00010000   # don't draw button label
IE_GUI_BUTTON_PLAYRANDOM = 0x00020000   # the button animation is random
IE_GUI_BUTTON_PLAYONCE   = 0x00040000   # the button animation won't restart
IE_GUI_BUTTON_PLAYALWAYS = 0x00080000   # animation will play when game is paused

IE_GUI_BUTTON_CENTER_PICTURES = 0x00100000 # center the button's PictureList
IE_GUI_BUTTON_BG1_PAPERDOLL   = 0x00200000 # BG1-style paperdoll
IE_GUI_BUTTON_HORIZONTAL      = 0x00400000 # horizontal clipping of overlay
IE_GUI_BUTTON_NO_TOOLTIP      = 0x00800000 # disable the tooltip

IE_GUI_BUTTON_PORTRAIT    = IE_GUI_BUTTON_PLAYONCE|IE_GUI_BUTTON_PLAYALWAYS|IE_GUI_BUTTON_PICTURE

#label flags
IE_GUI_LABEL_USE_COLOR = 1

#scrollbar flags
IE_GUI_SCROLLBAR_DEFAULT = 0x00000040   # mousewheel triggers it (same value as default button)

#textarea flags
IE_GUI_TEXTAREA_AUTOSCROLL = 1
IE_GUI_TEXTAREA_HISTORY = 2
IE_GUI_TEXTAREA_EDITABLE = 4

# TextArea font colors, see TextArea.h
TA_COLOR_NORMAL = 0
TA_COLOR_INITIALS = 1
TA_COLOR_BACKGROUND = 2
TA_COLOR_OPTIONS = 3
TA_COLOR_HOVER = 4
TA_COLOR_SELECTED = 5

ColorWhite = {'r' : 255, 'g' : 255, 'b' : 255, 'a' : 255}
ColorWhitish = {'r' : 215, 'g' : 215, 'b' : 215, 'a' : 255}
ColorRed = {'r' : 255, 'g' : 0, 'b' : 0, 'a' : 255}
ColorBlackish = {'r' : 33, 'g' : 44, 'b' : 44, 'a' : 205}
ColorGray = {'r' : 126, 'g' : 126, 'b' : 126, 'a' : 255}

#TextEdit
IE_GUI_TEXTEDIT_ALPHACHARS = 1
IE_GUI_TEXTEDIT_NUMERIC = 2

#gui control types
IE_GUI_BUTTON = 0
IE_GUI_PROGRESS = 1
IE_GUI_SLIDER = 2
IE_GUI_EDIT = 3
IE_GUI_TEXTAREA = 5
IE_GUI_LABEL = 6
IE_GUI_SCROLLBAR = 7
IE_GUI_WORLDMAP = 8
IE_GUI_MAP = 9
IE_GUI_CONSOLE = 10
IE_GUI_VIEW = 254

GEM_MB_ACTION               = 1
GEM_MB_MENU                 = 4

IE_ACT_MOUSE_PRESS   = 0
IE_ACT_MOUSE_DRAG    = 1
IE_ACT_MOUSE_OVER    = 2
IE_ACT_MOUSE_ENTER   = 3
IE_ACT_MOUSE_LEAVE   = 4
IE_ACT_VALUE_CHANGE  = 5
IE_ACT_DRAG_DROP_CRT = 6
IE_ACT_DRAG_DROP_SRC = 7
IE_ACT_DRAG_DROP_DST = 8
IE_ACT_CUSTOM        = 9

#button states
IE_GUI_BUTTON_ENABLED    = 0x00000000
IE_GUI_BUTTON_UNPRESSED  = 0x00000000
IE_GUI_BUTTON_PRESSED    = 0x00000001
IE_GUI_BUTTON_SELECTED   = 0x00000002
IE_GUI_BUTTON_DISABLED   = 0x00000003
# Like DISABLED, but processes MouseOver events and draws UNPRESSED bitmap
IE_GUI_BUTTON_LOCKED   = 0x00000004
# Draws DISABLED bitmap, but it isn't disabled
IE_GUI_BUTTON_FAKEDISABLED    = 0x00000005
# Draws PRESSED bitmap, but it isn't shifted
IE_GUI_BUTTON_FAKEPRESSED   = 0x00000006

#mapcontrol states (add 0x090000000 if used with SetControlStatus)
IE_GUI_MAP_NO_NOTES   =  0
IE_GUI_MAP_VIEW_NOTES =  1
IE_GUI_MAP_SET_NOTE   =  2
IE_GUI_MAP_REVEAL_MAP =  3

# !!! Keep these synchronized with Font.h !!!
IE_FONT_ALIGN_LEFT       = 0x00
IE_FONT_ALIGN_CENTER     = 0x01
IE_FONT_ALIGN_RIGHT      = 0x02
IE_FONT_ALIGN_BOTTOM     = 0x04
IE_FONT_ALIGN_TOP        = 0x10
IE_FONT_ALIGN_MIDDLE     = 0x20
IE_FONT_SINGLE_LINE      = 0x40

OP_SET = 0
OP_AND = 1
OP_OR = 2
OP_XOR = 3
OP_NAND = 4

# Window position anchors/alignments
# !!! Keep these synchronized with Window.h !!!
WF_DRAGGABLE		= 0x01
WF_BORDERLESS		= 0x02
WF_DESTROY_ON_CLOSE	= 0x04
WF_ALPHA_CHANNEL	= 0x08

WINDOW_TOP			 = 0x01
WINDOW_BOTTOM        = 0x02
WINDOW_VCENTER	     = 0x03
WINDOW_LEFT			 = 0x04
WINDOW_RIGHT         = 0x08
WINDOW_HCENTER       = 0xC
WINDOW_CENTER		 = 0xF

ACTION_WINDOW_CLOSED		= 0
ACTION_WINDOW_FOCUS_GAINED	= 1
ACTION_WINDOW_FOCUS_LOST	= 2

# animation flags
ANIM_PLAY_NORMAL		= 0,
ANIM_PLAY_RANDOM		= 1, # the button animation is random
ANIM_PLAY_ONCE			= 2, # the button animation won't restart
ANIM_PLAY_ALWAYS		= 4  # animation will play when game is paused

# GameScreen flags
GS_PARTYAI           = 1
GS_SMALLDIALOG       = 0
GS_MEDIUMDIALOG      = 2
GS_LARGEDIALOG       = 6
GS_DIALOGMASK        = 6
GS_DIALOG            = 8
GS_HIDEGUI           = 16
GS_OPTIONPANE        = 32
GS_PORTRAITPANE      = 64
GS_MAPNOTE           = 128

# GameControl screen flags
# !!! Keep these synchronized with GameControl.h !!!
SF_CENTERONACTOR     = 0
SF_ALWAYSCENTER      = 1

# GameControltarget modes
# !!! Keep these synchronized with GameControl.h !!!
TARGET_MODE_NONE    = 0
TARGET_MODE_TALK    = 1
TARGET_MODE_ATTACK  = 2
TARGET_MODE_CAST    = 3
TARGET_MODE_DEFEND  = 4
TARGET_MODE_PICK    = 5

GA_SELECT     = 16
GA_NO_DEAD    = 32
GA_POINT      = 64
GA_NO_HIDDEN  = 128
GA_NO_ALLY    = 256
GA_NO_ENEMY   = 512
GA_NO_NEUTRAL = 1024
GA_NO_SELF    = 2048

# Game features, for Interface::SetFeatures()
# Defined in globals.h
GF_ALL_STRINGS_TAGGED = 1

STRING_FLAGS_RESOLVE_TAGS = 16

# Shadow color for ShowModal()
# !!! Keep these synchronized with Interface.h !!!
MODAL_SHADOW_NONE = 0
MODAL_SHADOW_GRAY = 1
MODAL_SHADOW_BLACK = 2

# character resource directories
# !!! Keep these synchronized with Interface.h !!!
CHR_PORTRAITS = 0
CHR_SOUNDS = 1
CHR_EXPORTS = 2
CHR_SCRIPTS = 3

# Flags for GameSelectPC()
# !!! Keep these synchronized with Game.h !!!
SELECT_NORMAL  = 0x00
SELECT_REPLACE = 0x01
SELECT_QUIET   = 0x02

# Spell types
# !!! Keep these synchronized with Spellbook.h !!!
IE_SPELL_TYPE_PRIEST = 0
IE_SPELL_TYPE_WIZARD = 1
IE_SPELL_TYPE_INNATE = 2
IE_SPELL_TYPE_SONG = 3

# IWD2 spell types
IE_IWD2_SPELL_BARD = 0
IE_IWD2_SPELL_CLERIC = 1
IE_IWD2_SPELL_DRUID = 2
IE_IWD2_SPELL_PALADIN = 3
IE_IWD2_SPELL_RANGER = 4
IE_IWD2_SPELL_SORCERER = 5
IE_IWD2_SPELL_WIZARD = 6
IE_IWD2_SPELL_DOMAIN = 7
IE_IWD2_SPELL_INNATE = 8
IE_IWD2_SPELL_SONG = 9
IE_IWD2_SPELL_SHAPE = 10

# Item Flags bits
# !!! Keep these synchronized with Item.h !!!
IE_ITEM_CRITICAL     = 0x00000001
IE_ITEM_TWO_HANDED   = 0x00000002
IE_ITEM_MOVABLE      = 0x00000004
IE_ITEM_DISPLAYABLE  = 0x00000008
IE_ITEM_CURSED       = 0x00000010
IE_ITEM_NOT_COPYABLE = 0x00000020
IE_ITEM_MAGICAL      = 0x00000040
IE_ITEM_BOW          = 0x00000080
IE_ITEM_SILVER       = 0x00000100
IE_ITEM_COLD_IRON    = 0x00000200
IE_ITEM_STOLEN       = 0x00000400
IE_ITEM_CONVERSABLE  = 0x00000800
IE_ITEM_PULSATING    = 0x00001000
IE_ITEM_UNSELLABLE   = (IE_ITEM_CRITICAL | IE_ITEM_STOLEN)

# CREItem (SlotItem) Flags bits
# !!! Keep these synchronized with Inventory.h !!!
IE_INV_ITEM_IDENTIFIED    = 0x01
IE_INV_ITEM_UNSTEALABLE   = 0x02
IE_INV_ITEM_STOLEN        = 0x04
IE_INV_ITEM_STEEL         = 0x04 # pst only
IE_INV_ITEM_UNDROPPABLE   = 0x08
# GemRB extensions
IE_INV_ITEM_ACQUIRED      = 0x10
IE_INV_ITEM_DESTRUCTIBLE  = 0x20
IE_INV_ITEM_EQUIPPED      = 0x40
IE_INV_ITEM_STACKED       = 0x80
# these come from the original item bits
IE_INV_ITEM_CRITICAL      = 0x100
IE_INV_ITEM_TWOHANDED     = 0x200
IE_INV_ITEM_MOVABLE       = 0x400
IE_INV_ITEM_UNKNOWN800    = 0x800
IE_INV_ITEM_CURSED        = 0x1000
IE_INV_ITEM_UNKNOWN2000   = 0x2000
IE_INV_ITEM_MAGICAL       = 0x4000
IE_INV_ITEM_BOW           = 0x8000
IE_INV_ITEM_SILVER        = 0x10000
IE_INV_ITEM_COLDIRON      = 0x20000
IE_INV_ITEM_STOLEN2       = 0x40000
IE_INV_ITEM_CONVERSABLE   = 0x80000
IE_INV_ITEM_PULSATING     = 0x100000

# !!! Keep this synchronized with Store.h !!!
SHOP_BUY    = 1
SHOP_SELL   = 2
SHOP_ID     = 4
SHOP_STEAL  = 8
SHOP_SELECT = 0x40
SHOP_NOREPADJ = 0x2000 # IE_STORE_NOREPADJ
SHOP_FULL   = 0x10000 # IE_STORE_CAPACITY

#game constants

# !!! Keep this synchronized with Video.h !!!
TOOLTIP_DELAY_FACTOR = 250

#game strings
STR_LOADMOS  = 0
STR_AREANAME = 1
STR_TEXTSCREEN = 2

#game integers
SV_BPP = 0
SV_WIDTH = 1
SV_HEIGHT = 2
SV_GAMEPATH = 3
SV_TOUCH = 4
SV_SAVEPATH = 5

# GUIEnhancements bits
GE_SCROLLBARS = 1
GE_TRY_IDENTIFY_ON_TRANSFER = 2
GE_ALWAYS_OPEN_CONTAINER_ITEMS = 4
GE_UNFOCUS_STOPS_SCROLLING = 8

# Log Levels
# !!! Keep this synchronized with Logging/Logging.h !!!
# no need for LOG_INTERNAL here since its internal to the logger class
LOG_NONE = -1 # here just for the scripts, not needed in core
LOG_FATAL = 0
LOG_ERROR = 1
LOG_WARNING = 2
LOG_MESSAGE = 3
LOG_COMBAT = 4
LOG_DEBUG = 5

# GetTableValue return modes
GTV_STR = 0
GTV_INT = 1
GTV_STAT = 2
GTV_REF = 3

# UpdateActionsWindow action levels
UAW_STANDARD = 0
UAW_EQUIPMENT = 1
UAW_SPELLS = 2
UAW_INNATES = 3
UAW_QWEAPONS = 4
UAW_ALLMAGE = 5
UAW_SKILLS = 6
UAW_QSPELLS = 7
UAW_QSHAPES = 8
UAW_QSONGS = 9
UAW_BOOK = 10
UAW_2DASPELLS = 11
UAW_SPELLS_DIRECT = 12
UAW_QITEMS = 13

# item extended header location field
ITEM_LOC_WEAPON = 1  # show on quick weapon ability selection
ITEM_LOC_EQUIPMENT = 3 # show on quick item ability selection

# item function types
ITM_F_DRINK = 1
ITM_F_READ = 2
ITM_F_CONTAINER = 4
ITM_F_ABILITIES = 8

# modifier keys - keep in sync with EventMgr.h
GEM_MOD_SHIFT = 1
GEM_MOD_CTRL = 2
GEM_MOD_ALT = 4
