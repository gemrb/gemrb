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
#Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.


import _GemRB

from MetaClasses import metaIDWrapper, metaControl
#from exceptions import RuntimeError

class GTable:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetValue': _GemRB.GetTableValue,
    'FindValue': _GemRB.FindTableValue,
    'GetRowIndex': _GemRB.GetTableRowIndex,
    'GetRowName': _GemRB.GetTableRowName,
    'GetColumnIndex': _GemRB.GetTableColumnIndex,
    'GetColumnName': _GemRB.GetTableColumnName,
    'GetRowCount': _GemRB.GetTableRowCount,
    'GetColumnCount': _GemRB.GetTableColumnCount
  }
  def __del__(self):
    # don't unload tables if the _GemRB module is already unloaded at exit
    if self.ID != -1 and _GemRB:
      _GemRB.UnloadTable(self.ID)
  def __nonzero__(self):
    return self.ID != -1

class GSymbol:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetValue': _GemRB.GetSymbolValue,
    'Unload': _GemRB.UnloadSymbol
  }

class GWindow:
  __metaclass__ = metaIDWrapper
  methods = {
    'SetSize': _GemRB.SetWindowSize,
    'SetFrame': _GemRB.SetWindowFrame,
    'SetPicture': _GemRB.SetWindowPicture,
    'SetPos': _GemRB.SetWindowPos,
    'HasControl': _GemRB.HasControl,
    'DeleteControl': _GemRB.DeleteControl,
    'Unload': _GemRB.UnloadWindow,
    'SetupEquipmentIcons': _GemRB.SetupEquipmentIcons,
    'SetupSpellIcons': _GemRB.SetupSpellIcons,
    'SetupControls': _GemRB.SetupControls,
    'SetVisible': _GemRB.SetVisible,
    'ShowModal': _GemRB.ShowModal,
    'Invalidate': _GemRB.InvalidateWindow
  }
  def GetControl(self, control):
    return _GemRB.GetControlObject(self.ID, control)
  def CreateWorldMapControl(self, control, *args):
    _GemRB.CreateWorldMapControl(self.ID, control, *args)
    return _GemRB.GetControlObject(self.ID, control)
  def CreateMapControl(self, control, *args):
    _GemRB.CreateMapControl(self.ID, control, *args)
    return _GemRB.GetControlObject(self.ID, control)
  def CreateLabel(self, control, *args):
    _GemRB.CreateLabel(self.ID, control, *args)
    return _GemRB.GetControlObject(self.ID, control)
  def CreateButton(self, control, *args):
    _GemRB.CreateButton(self.ID, control, *args)
    return _GemRB.GetControlObject(self.ID, control)
  def CreateScrollBar(self, control, *args):
    _GemRB.CreateScrollBar(self.ID, control, *args)
    return _GemRB.GetControlObject(self.ID, control)
  def CreateTextEdit(self, control, *args):
    _GemRB.CreateTextEdit(self.ID, control, *args)
    return _GemRB.GetControlObject(self.ID, control)
 

class GControl:
  __metaclass__ = metaControl
  methods = {
    'SetVarAssoc': _GemRB.SetVarAssoc,
    'SetPos': _GemRB.SetControlPos,
    'SetSize': _GemRB.SetControlSize,
    'SetAnimationPalette': _GemRB.SetAnimationPalette,
    'SetAnimation': _GemRB.SetAnimation,
    'QueryText': _GemRB.QueryText,
    'SetText': _GemRB.SetText,
    'SetTooltip': _GemRB.SetTooltip,
    'SetEvent': _GemRB.SetEvent,
    'SetStatus': _GemRB.SetControlStatus,
  }
  def AttachScrollBar(self, scrollbar):
    if self.WinID != scrollbar.WinID:
      raise RuntimeError, "Scrollbar must be in same Window as Control"
    return _GemRB.AttachScrollBar(self.WinID, self.ID, scrollbar.ID)

class GLabel(GControl):
  __metaclass__ = metaControl
  methods = {
    'SetTextColor': _GemRB.SetLabelTextColor,
    'SetUseRGB': _GemRB.SetLabelUseRGB
  }

class GTextArea(GControl):
  __metaclass__ = metaControl
  methods = {
    'Rewind': _GemRB.RewindTA,
    'SetHistory': _GemRB.SetTAHistory,
    'Append': _GemRB.TextAreaAppend,
    'Clear': _GemRB.TextAreaClear,
    'Scroll': _GemRB.TextAreaScroll,
    'SetFlags': _GemRB.SetTextAreaFlags,
    'GetCharSounds': _GemRB.GetCharSounds,
    'GetCharacters': _GemRB.GetCharacters,
    'GetPortraits': _GemRB.GetPortraits
  }
  def MoveText(self, other):
    _GemRB.MoveTAText(self.WinID, self.ID, other.WinID, other.ID)

class GTextEdit(GControl):
  __metaclass__ = metaControl
  methods = {
    'SetBufferLength': _GemRB.SetBufferLength
  }
  def ConvertEdit(self, ScrollBarID):
    newID = _GemRB.ConvertEdit(self.WinID, self.ID, ScrollBarID)
    return _GemRB.GetControlObject(self.WinID, newID)

class GScrollBar(GControl):
  __metaclass__ = metaControl
  methods = {
    'SetDefaultScrollBar': _GemRB.SetDefaultScrollBar,
    'SetSprites': _GemRB.SetScrollBarSprites
  }

class GButton(GControl):
  __metaclass__ = metaControl
  methods = {
    'SetSprites': _GemRB.SetButtonSprites,
    'SetOverlay': _GemRB.SetButtonOverlay,
    'SetBorder': _GemRB.SetButtonBorder,
    'EnableBorder': _GemRB.EnableButtonBorder,
    'SetFont': _GemRB.SetButtonFont,
    'SetTextColor': _GemRB.SetButtonTextColor,
    'SetFlags': _GemRB.SetButtonFlags,
    'SetState': _GemRB.SetButtonState,
    'SetPictureClipping': _GemRB.SetButtonPictureClipping,
    'SetPicture': _GemRB.SetButtonPicture,
    'SetMOS': _GemRB.SetButtonMOS,
    'SetPLT': _GemRB.SetButtonPLT,
    'SetBAM': _GemRB.SetButtonBAM,
    'SetSaveGamePortrait': _GemRB.SetSaveGamePortrait,
    'SetSaveGamePreview': _GemRB.SetSaveGamePreview,
    'SetGamePreview': _GemRB.SetGamePreview,
    'SetGamePortraitPreview': _GemRB.SetGamePortraitPreview,
    'SetSpellIcon': _GemRB.SetSpellIcon,
    'SetItemIcon': _GemRB.SetItemIcon,
    'SetActionIcon': _GemRB.SetActionIcon
  }
  def CreateLabelOnButton(self, control, *args):
    _GemRB.CreateLabelOnButton(self.WinID, self.ID, control, *args)
    return _GemRB.GetControlObject(self.WinID, control)

class GWorldMap(GControl):
  __metaclass__ = metaControl
  methods = {
    'AdjustScrolling': _GemRB.AdjustScrolling,
    'GetDestinationArea': _GemRB.GetDestinationArea,
    'SetTextColor': _GemRB.SetWorldMapTextColor
  }

