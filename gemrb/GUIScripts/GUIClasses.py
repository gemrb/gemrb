#-*-python-*-
#GemRB - Infinity Engine Emulator
#Copyright (C) 2009 The GemRB Project
#
#This program is free software; you can redistribute it and/or
#modify it under the terms of the GNU General Public License
#as published by the Free Software Foundation; either version 2
#of the License, or (at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program; if not, write to the Free Software
#Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# $Id$

import GemRB

from MetaClasses import metaIDWrapper, metaControl
#from exceptions import RuntimeError

class GTable:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetValue': GemRB.GetTableValue,
    'FindValue': GemRB.FindTableValue,
    'GetRowIndex': GemRB.GetTableRowIndex,
    'GetRowName': GemRB.GetTableRowName,
    'GetColumnIndex': GemRB.GetTableColumnIndex,
    'GetColumnName': GemRB.GetTableColumnName,
    'GetRowCount': GemRB.GetTableRowCount,
    'GetColumnCount': GemRB.GetTableColumnCount
  }
  def __del__(self):
    # don't unload tables if the GemRB module is already unloaded at exit
    if self.ID != -1 and GemRB:
      GemRB.UnloadTable(self.ID)
  def __nonzero__(self):
    return self.ID != -1

class GSymbol:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetValue': GemRB.GetSymbolValue,
    'Unload': GemRB.UnloadSymbol
  }

class GWindow:
  __metaclass__ = metaIDWrapper
  methods = {
    'SetSize': GemRB.SetWindowSize,
    'SetFrame': GemRB.SetWindowFrame,
    'SetPicture': GemRB.SetWindowPicture,
    'SetPos': GemRB.SetWindowPos,
    'HasControl': GemRB.HasControl,
    'DeleteControl': GemRB.DeleteControl,
    'Unload': GemRB.UnloadWindow,
    'SetupEquipmentIcons': GemRB.SetupEquipmentIcons,
    'SetupSpellIcons': GemRB.SetupSpellIcons,
    'SetupControls': GemRB.SetupControls,
    'SetVisible': GemRB.SetVisible,
    'ShowModal': GemRB.ShowModal,
    'Invalidate': GemRB.InvalidateWindow
  }
  def GetControl(self, control):
    return GemRB.GetControlObject(self.ID, control)
  def CreateWorldMapControl(self, control, *args):
    GemRB.CreateWorldMapControl(self.ID, control, *args)
    return GemRB.GetControlObject(self.ID, control)
  def CreateMapControl(self, control, *args):
    GemRB.CreateMapControl(self.ID, control, *args)
    return GemRB.GetControlObject(self.ID, control)
  def CreateLabel(self, control, *args):
    GemRB.CreateLabel(self.ID, control, *args)
    return GemRB.GetControlObject(self.ID, control)
  def CreateButton(self, control, *args):
    GemRB.CreateButton(self.ID, control, *args)
    return GemRB.GetControlObject(self.ID, control)
  def CreateTextEdit(self, control, *args):
    GemRB.CreateTextEdit(self.ID, control, *args)
    return GemRB.GetControlObject(self.ID, control)
 

class GControl:
  __metaclass__ = metaControl
  methods = {
    'SetVarAssoc': GemRB.SetVarAssoc,
    'SetPos': GemRB.SetControlPos,
    'SetSize': GemRB.SetControlSize,
    'SetDefaultScrollBar': GemRB.SetDefaultScrollBar,
    'SetAnimationPalette': GemRB.SetAnimationPalette,
    'SetAnimation': GemRB.SetAnimation,
    'QueryText': GemRB.QueryText,
    'SetText': GemRB.SetText,
    'SetTooltip': GemRB.SetTooltip,
    'SetEvent': GemRB.SetEvent,
    'SetStatus': GemRB.SetControlStatus,
  }
  def AttachScrollBar(self, scrollbar):
    if self.WinID != scrollbar.WinID:
      raise RuntimeError, "Scrollbar must be in same Window as Control"
    return GemRB.AttachScrollBar(self.WinID, self.ID, scrollbar.ID)

class GLabel(GControl):
  __metaclass__ = metaControl
  methods = {
    'SetTextColor': GemRB.SetLabelTextColor,
    'SetUseRGB': GemRB.SetLabelUseRGB
  }

class GTextArea(GControl):
  __metaclass__ = metaControl
  methods = {
    'Rewind': GemRB.RewindTA,
    'SetHistory': GemRB.SetTAHistory,
    'Append': GemRB.TextAreaAppend,
    'Clear': GemRB.TextAreaClear,
    'Scroll': GemRB.TextAreaScroll,
    'SetFlags': GemRB.SetTextAreaFlags,
    'GetCharSounds': GemRB.GetCharSounds,
    'GetCharacters': GemRB.GetCharacters,
    'GetPortraits': GemRB.GetPortraits
  }
  def MoveText(self, other):
    GemRB.MoveTAText(self.WinID, self.ID, other.WinID, other.ID)

class GTextEdit(GControl):
  __metaclass__ = metaControl
  methods = {
    'SetBufferLength': GemRB.SetBufferLength
  }
 
class GButton(GControl):
  __metaclass__ = metaControl
  methods = {
    'SetSprites': GemRB.SetButtonSprites,
    'SetOverlay': GemRB.SetButtonOverlay,
    'SetBorder': GemRB.SetButtonBorder,
    'EnableBorder': GemRB.EnableButtonBorder,
    'SetFont': GemRB.SetButtonFont,
    'SetTextColor': GemRB.SetButtonTextColor,
    'SetFlags': GemRB.SetButtonFlags,
    'SetState': GemRB.SetButtonState,
    'SetPictureClipping': GemRB.SetButtonPictureClipping,
    'SetPicture': GemRB.SetButtonPicture,
    'SetMOS': GemRB.SetButtonMOS,
    'SetPLT': GemRB.SetButtonPLT,
    'SetBAM': GemRB.SetButtonBAM,
    'SetSaveGamePortrait': GemRB.SetSaveGamePortrait,
    'SetSaveGamePreview': GemRB.SetSaveGamePreview,
    'SetGamePreview': GemRB.SetGamePreview,
    'SetGamePortraitPreview': GemRB.SetGamePortraitPreview,
    'SetSpellIcon': GemRB.SetSpellIcon,
    'SetItemIcon': GemRB.SetItemIcon,
    'SetActionIcon': GemRB.SetActionIcon
  }
  def CreateLabelOnButton(self, control, *args):
    GemRB.CreateLabelOnButton(self.WinID, self.ID, control, *args)
    return GemRB.GetControlObject(self.WinID, control)

class GWorldMap(GControl):
  __metaclass__ = metaControl
  methods = {
    'AdjustScrolling': GemRB.AdjustScrolling,
    'GetDestinationArea': GemRB.GetDestinationArea,
    'SetTextColor': GemRB.SetWorldMapTextColor
  }

