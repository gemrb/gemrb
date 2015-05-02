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
import CreateControlDecorators

from MetaClasses import metaIDWrapper, metaControl

def CreateControlDecorator(func):
	wrapper = getattr(CreateControlDecorators, func.__name__, None)
	if wrapper:
		return wrapper(func)
	return func # unchanged, no wrapper exists

class GTable:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetValue': _GemRB.Table_GetValue,
    'FindValue': _GemRB.Table_FindValue,
    'GetRowIndex': _GemRB.Table_GetRowIndex,
    'GetRowName': _GemRB.Table_GetRowName,
    'GetColumnIndex': _GemRB.Table_GetColumnIndex,
    'GetColumnName': _GemRB.Table_GetColumnName,
    'GetRowCount': _GemRB.Table_GetRowCount,
    'GetColumnCount': _GemRB.Table_GetColumnCount
  }
  def __del__(self):
    # don't unload tables if the _GemRB module is already unloaded at exit
    if self.ID != -1 and _GemRB:
      pass #_GemRB.Table_Unload(self.ID)
  def __nonzero__(self):
    return self.ID != -1

class GSymbol:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetValue': _GemRB.Symbol_GetValue,
    'Unload': _GemRB.Symbol_Unload
  }
  
class GView:
	__metaclass__ = metaIDWrapper
	methods = {
	'GetFrame': _GemRB.View_GetFrame,
    'SetFrame': _GemRB.View_SetFrame,
    'SetBackground': _GemRB.View_SetBackground,
	}
	def SetSize(self, w, h):
		r = self.GetFrame()
		self.SetFrame(r['x'], r['y'], w, h);

	def SetPos(self, x, y):
		r = self.GetFrame()
		self.SetFrame(x, y, r['w'], r['h']);

class GWindow(GView):
  methods = {
    'HasControl': _GemRB.Window_HasControl,
    'DeleteControl': _GemRB.Window_DeleteControl,
    'SetupEquipmentIcons': _GemRB.Window_SetupEquipmentIcons,
    'SetupControls': _GemRB.Window_SetupControls,
    'SetVisible': _GemRB.Window_SetVisible,
    'ShowModal': _GemRB.Window_ShowModal,
    'Invalidate': _GemRB.Window_Invalidate,
  }
  def SetBackground(self, resref):
	  _GemRB.View_SetBackground(self.ID, -1, resref)

  def __nonzero__(self):
    return self.ID != -1
  def Unload(self):
    if self.ID != -1:
      _GemRB.Window_Unload(self.ID)
      self.ID = -1
  def GetControl(self, control):
    return _GemRB.Window_GetControl(self.ID, control)
  @CreateControlDecorator
  def CreateWorldMapControl(self, control, *args):
    _GemRB.Window_CreateWorldMapControl(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  @CreateControlDecorator
  def CreateMapControl(self, control, *args):
    _GemRB.Window_CreateMapControl(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  @CreateControlDecorator
  def CreateLabel(self, control, *args):
    _GemRB.Window_CreateLabel(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  @CreateControlDecorator
  def CreateButton(self, control, *args):
    _GemRB.Window_CreateButton(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  @CreateControlDecorator
  def CreateScrollBar(self, control, *args):
    _GemRB.Window_CreateScrollBar(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  @CreateControlDecorator
  def CreateTextArea(self, control, *args):
    _GemRB.Window_CreateTextArea(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)    
  @CreateControlDecorator
  def CreateTextEdit(self, control, *args):
    _GemRB.Window_CreateTextEdit(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
 

class GControl(GView):
  __metaclass__ = metaControl
  methods = {
    'HasAnimation': _GemRB.Control_HasAnimation,
    'SetVarAssoc': _GemRB.Control_SetVarAssoc,
    'SetAnimationPalette': _GemRB.Control_SetAnimationPalette,
    'SetAnimation': _GemRB.Control_SetAnimation,
    'QueryText': _GemRB.Control_QueryText,
    'SetText': _GemRB.Control_SetText,
    'SetTooltip': _GemRB.Control_SetTooltip,
    'SetEvent': _GemRB.Control_SetEvent,
    'SetStatus': _GemRB.Control_SetStatus,
  }
  def AttachScrollBar(self, scrollbar):
    if self.WinID != scrollbar.WinID:
      raise RuntimeError, "Scrollbar must be in same Window as Control"
    return _GemRB.Control_AttachScrollBar(self.WinID, self.ID, scrollbar.ID)
  def SubstituteForControl(self, target):
	  return _GemRB.Control_SubstituteForControl(self.WinID, self.ID, target.WinID, target.ID)

class GLabel(GControl):
  methods = {
    'SetFont': _GemRB.Label_SetFont,
    'SetTextColor': _GemRB.Label_SetTextColor,
    'SetUseRGB': _GemRB.Label_SetUseRGB
  }

class GTextArea(GControl):
  methods = {
    'ChapterText': _GemRB.TextArea_SetChapterText,
    'Append': _GemRB.TextArea_Append,
    'Clear': _GemRB.TextArea_Clear,
    'SetFlags': _GemRB.Control_TextArea_SetFlags,
    'SetOptions': _GemRB.TextArea_SetOptions,
    'ListResources': _GemRB.TextArea_ListResources
  }
  __slots__ = ['DefaultText']

class GTextEdit(GControl):
  methods = {
    'SetBufferLength': _GemRB.TextEdit_SetBufferLength,
  }

class GScrollBar(GControl):
  methods = {
    'SetDefaultScrollBar': _GemRB.ScrollBar_SetDefaultScrollBar
  }

class GButton(GControl):
  methods = {
    'SetSprites': _GemRB.Button_SetSprites,
    'SetOverlay': _GemRB.Button_SetOverlay,
    'SetBorder': _GemRB.Button_SetBorder,
    'EnableBorder': _GemRB.Button_EnableBorder,
    'SetFont': _GemRB.Button_SetFont,
    'SetAnchor': _GemRB.Button_SetAnchor,
    'SetPushOffset': _GemRB.Button_SetPushOffset,
    'SetTextColor': _GemRB.Button_SetTextColor,
    'SetFlags': _GemRB.Button_SetFlags,
    'SetState': _GemRB.Button_SetState,
    'SetPictureClipping': _GemRB.Button_SetPictureClipping,
    'SetPicture': _GemRB.Button_SetPicture,
    'SetSprite2D': _GemRB.Button_SetSprite2D,
    'SetMOS': _GemRB.Button_SetMOS,
    'SetPLT': _GemRB.Button_SetPLT,
    'SetBAM': _GemRB.Button_SetBAM,
    'SetSpellIcon': _GemRB.Button_SetSpellIcon,
    'SetItemIcon': _GemRB.Button_SetItemIcon,
    'SetActionIcon': _GemRB.Button_SetActionIcon
  }
  def CreateLabelOnButton(self, control, *args):
    _GemRB.Button_CreateLabelOnButton(self.WinID, self.ID, control, *args)
    return _GemRB.Window_GetControl(self.WinID, control)

class GWorldMap(GControl):
  methods = {
    'AdjustScrolling': _GemRB.WorldMap_AdjustScrolling,
    'GetDestinationArea': _GemRB.WorldMap_GetDestinationArea,
    'SetTextColor': _GemRB.WorldMap_SetTextColor
  }

class GSaveGame:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetDate': _GemRB.SaveGame_GetDate,
    'GetGameDate': _GemRB.SaveGame_GetGameDate,
    'GetName': _GemRB.SaveGame_GetName,
    'GetPortrait': _GemRB.SaveGame_GetPortrait,
    'GetPreview': _GemRB.SaveGame_GetPreview,
    'GetSaveID': _GemRB.SaveGame_GetSaveID,
  }

class GSprite2D:
  __metaclass__ = metaIDWrapper
  methods = {}
