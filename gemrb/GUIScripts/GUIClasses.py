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
import GemRB
import GameCheck

from GUIDefines import *
from MetaClasses import metaIDWrapper, add_metaclass
from GemRB import GetView, CreateView, RemoveView, RemoveScriptingRef

DefaultScrollbars = {
	"bg1": "GUIWSBR",
	"bg2": "GUISCRCW",
	"bg2ee": "GUISCRCW",
	"iwd": "GUISBR",
	"how": "GUISBR",
	"iwd2": "GBTNSCRL",
	"pst": "CGSCRL1",
	"demo": "scrlbar1"
}

def CreateScrollbarARGs(bam = None):
	bamframes = list(range(6))
	if GameCheck.IsBG2():
		bamframes = [0,1,2,3,5,4]
	elif GameCheck.IsBG1():
		bamframes = [0,1,2,3,6,7]

	if not bam:
		fallbackBAM = "scrlbar1"
		if GemRB.GameType in DefaultScrollbars:
			fallbackBAM = DefaultScrollbars[GemRB.GameType]
		bam = fallbackBAM

	return (bam, bamframes)

def _ExtractFrame(args):
	return { "x" : args[0], "y" : args[1], "w" : args[2], "h" : args[3] }

@add_metaclass(metaIDWrapper)
class GTable:
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

	def __bool__(self):
		return self.ID != -1

@add_metaclass(metaIDWrapper)
class GSymbol:
	methods = {
		'GetValue': _GemRB.Symbol_GetValue
	}

class Scrollable(object):
	def Scroll(self, x, y, relative = True):
		_GemRB.Scrollable_Scroll(self, x, y, relative)

	def ScrollTo(self, x, y):
		self.Scroll(x, y, False)

	def ScrollDelta(self, x, y):
		self.Scroll(x, y, True)

@add_metaclass(metaIDWrapper)
class GView:
	methods = {
	'AddAlias': _GemRB.View_AddAlias,
	'AddSubview': _GemRB.View_AddSubview,
	'SetEventProxy': _GemRB.View_SetEventProxy,
	'GetFrame': _GemRB.View_GetFrame,
	'SetFrame': _GemRB.View_SetFrame,
	'SetBackground': _GemRB.View_SetBackground,
	'SetFlags': _GemRB.View_SetFlags,
	'SetResizeFlags': _GemRB.View_SetResizeFlags,
	'SetTooltip': _GemRB.View_SetTooltip,
	'Focus': _GemRB.View_Focus
	}

	__slots__ = ['SCRIPT_GROUP', 'Flags', 'Window']
	
	def __eq__(self, rhs):
		if rhs == None:
			return self.ID == -1
		return self.ID == rhs.ID and self.SCRIPT_GROUP == rhs.SCRIPT_GROUP
	
	def __hash__(self):
		return self.SCRIPT_GROUP + str(self.ID)
	
	def __bool__(self):
		return self.ID != -1

	def GetSize(self):
		frame = self.GetFrame()
		return (frame['w'], frame['h'])

	def GetPos(self):
		frame = self.GetFrame()
		return (frame['x'], frame['y'])

	def SetSize(self, w, h):
		r = self.GetFrame()
		r['w'] = w
		r['h'] = h
		self.SetFrame(r);

	def SetPos(self, x, y):
		r = self.GetFrame()
		r['x'] = x
		r['y'] = y
		self.SetFrame(r);

	def GetInsetFrame(self, l = 0, r = None, t = None, b = None):
		r = r if r is not None else l
		t = t if t is not None else l
		b = b if b is not None else t
        
		frame = self.GetFrame()
		frame['x'] = l
		frame['y'] = b
		frame['w'] -= (l + r)
		frame['h'] -= (b + t)
		return frame

	def SetVisible(self, visible):
		self.SetFlags(IE_GUI_VIEW_INVISIBLE, OP_NAND if visible else OP_OR)
		
	def IsVisible(self):
		return not (self.Flags & IE_GUI_VIEW_INVISIBLE)

	def SetDisabled(self, disable):
		self.SetFlags(IE_GUI_VIEW_DISABLED, OP_OR if disable else OP_NAND)

	def ReplaceSubview(self, subview, ctype, *args):
		if isinstance(subview, int):
			subview = self.GetControl (subview)
		newID = subview.ID & 0x00000000ffffffff
		frame = subview.GetFrame()
		RemoveView(subview, True)

		return self.CreateSubview(newID, ctype, frame, args)

	def CreateSubview(self, newID, ctype, frame, *args):
		view = CreateView(newID, ctype, frame, *args) # this will create an entry in the generic 'control' group
		created = self.AddSubview(view) # this will move the reference into the our window's group
		RemoveScriptingRef(view) # destroy the old reference just in case something tries to recycle the id while 'created' is still valid
		return created

	def RemoveSubview(self, view, delete=False):
		return RemoveView(view, delete)

	def CreateWorldMapControl(self, control, *args):
		frame = _ExtractFrame(args[0:4])
		return self.CreateSubview(control, IE_GUI_WORLDMAP, frame, args[4:])
	
	def CreateMapControl(self, control, *args):
		frame = _ExtractFrame(args[0:4])
		return self.CreateSubview(control, IE_GUI_MAP, frame, args[4:])

	def CreateLabel(self, control, *args):
		frame = _ExtractFrame(args[0:4])
		return self.CreateSubview(control, IE_GUI_LABEL, frame, args[4:])

	def CreateButton(self, control, *args):
		frame = _ExtractFrame(args[0:4])
		return self.CreateSubview(control, IE_GUI_BUTTON, frame, args[4:])

	def CreateScrollBar(self, control, frame, bam=None):
		view = CreateView (control, IE_GUI_SCROLLBAR, frame, CreateScrollbarARGs(bam))
		return self.AddSubview (view)

	def CreateTextArea(self, control, *args):
		frame = _ExtractFrame(args[0:4])
		return self.CreateSubview(control, IE_GUI_TEXTAREA, frame, args[4:])

class GWindow(GView, Scrollable):
	methods = {
		'SetupEquipmentIcons': _GemRB.Window_SetupEquipmentIcons,
		'SetupControls': _GemRB.Window_SetupControls,
		'Focus': _GemRB.Window_Focus,
		'ShowModal': _GemRB.Window_ShowModal,
		'SetAction': _GemRB.Window_SetAction
	}

	__slots__ = ['HasFocus']

	def DeleteControl(self, view):
		if type(view) == int:
			view = self.GetControl (view)
		RemoveView (view, True)

	def GetControl(self, newID):
		return GetView(self, newID)

	def AliasControls (self, map):
		for alias, cid in map.items():
			control = self.GetControl(cid)
			if control:
				control.AddAlias(alias, self.ID)
			else:
				print("no control with id=" + str(cid))

	def GetControlAlias(self, alias): # see AliasControls()
		return GetView(alias, self.ID)

	def ReparentSubview(self, view, newparent):
		# reparenting assumes within the same window
		newID = view.ID & 0x00000000ffffffff
		view = self.RemoveSubview(view)

		parentFrame = newparent.GetFrame()
		frame = view.GetFrame()
		frame['x'] -= parentFrame['x']
		frame['y'] -= parentFrame['y']
		view.SetFrame(frame)
		return newparent.AddSubview(view, None, newID)

	def Close(self, *args):
		RemoveView(self, False)

class GControl(GView):
	methods = {
		'SetVarAssoc': _GemRB.Control_SetVarAssoc,
		'QueryText': _GemRB.Control_QueryText,
		'SetText': _GemRB.Control_SetText,
		'SetAction': _GemRB.Control_SetAction,
		'SetActionInterval': _GemRB.Control_SetActionInterval,
		'SetColor': _GemRB.Control_SetColor,
		'SetStatus': _GemRB.Control_SetStatus,
		'SetValue': _GemRB.Control_SetValue
	}

	__slots__ = ['ControlID', 'VarName', 'Value']

	def OnMouseEnter(self, handler):
		self.SetAction(handler, IE_ACT_MOUSE_ENTER)

	def OnMouseLeave(self, handler):
		self.SetAction(handler, IE_ACT_MOUSE_LEAVE)

	def QueryInteger(self):
		return int("0"+self.QueryText())
		
	def OnPress(self, handler):
		self.SetAction(handler, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, 0, 1)

	def OnRightPress(self, handler):
		self.SetAction(handler, IE_ACT_MOUSE_PRESS, GEM_MB_MENU, 0, 1)

	def OnShiftPress(self, handler):
		self.SetAction(handler, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, 1, 1)

	def OnDoublePress(self, handler):
		self.SetAction(handler, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, 0, 2)
		
	def OnChange(self, handler):
		self.SetAction(handler, IE_ACT_VALUE_CHANGE)

class GLabel(GControl):
	methods = {
		'SetFont': _GemRB.Label_SetFont,
	}

class GTextArea(GControl, Scrollable):
	methods = {
		'SetChapterText': _GemRB.TextArea_SetChapterText,
		'Append': _GemRB.TextArea_Append,
	}
	__slots__ = ['DefaultText']

	def ListResources(self, what, opts=0):
		return _GemRB.TextArea_ListResources(self, what, opts)

	def Clear(self):
		self.SetText("")

	def SetOptions(self, optList, varname=None, val=0):
		_GemRB.TextArea_SetOptions(self, optList)
		if varname:
			self.SetVarAssoc(varname, val)

	def OnSelect(self, handler):
		self.SetAction(handler, IE_ACT_CUSTOM)

class GTextEdit(GControl):
	methods = {
		'SetBufferLength': _GemRB.TextEdit_SetBufferLength,
	}

	def OnDone(self, handler):
		self.SetAction(handler, IE_ACT_CUSTOM)

	def OnCancel(self, handler):
		self.SetAction(handler, IE_ACT_CUSTOM + 1)

class GScrollBar(GControl, Scrollable):
	def SetVarAssoc(self, varname, val, rangeMin = 0, rangeMax = None):
		rangeMax = val if rangeMax is None else rangeMax
		super(GScrollBar, self).SetVarAssoc(varname, val, rangeMin, rangeMax)

class GButton(GControl):
	methods = {
		'SetSprites': _GemRB.Button_SetSprites,
		'SetOverlay': _GemRB.Button_SetOverlay,
		'SetBorder': _GemRB.Button_SetBorder,
		'EnableBorder': _GemRB.Button_EnableBorder,
		'SetFont': _GemRB.Button_SetFont,
		'SetHotKey': _GemRB.Button_SetHotKey,
		'SetAnchor': _GemRB.Button_SetAnchor,
		'SetPushOffset': _GemRB.Button_SetPushOffset,
		'SetState': _GemRB.Button_SetState,
		'SetPictureClipping': _GemRB.Button_SetPictureClipping,
		'SetPicture': _GemRB.Button_SetPicture,
		'SetPLT': _GemRB.Button_SetPLT,
		'SetBAM': _GemRB.Button_SetBAM,
		'SetSpellIcon': _GemRB.Button_SetSpellIcon,
		'SetItemIcon': _GemRB.Button_SetItemIcon,
		'SetActionIcon': _GemRB.Button_SetActionIcon,
		'SetAnimation': _GemRB.Button_SetAnimation,
	}

	def MakeDefault(self, glob=False):
		# return key
		return self.SetHotKey(chr(0x86), 0, glob)

	def MakeEscape(self, glob=False):
		# escape key
		return self.SetHotKey(chr(0x8c), 0, glob)

	def CreateLabel(self, labelid, *args):
		frame = self.GetFrame()
		frame["x"] = frame["y"] = 0
		return self.CreateSubview(labelid, IE_GUI_LABEL, frame, args)
		
	def CreateButton(self, btnid):
		frame = self.GetFrame()
		frame["x"] = frame["y"] = 0
		return self.CreateSubview(btnid, IE_GUI_BUTTON, frame)

class GWorldMap(GControl, Scrollable):
	methods = {
		'GetDestinationArea': _GemRB.WorldMap_GetDestinationArea
	}

class GMap(GControl):
	pass

class GProgressBar(GControl):
	def OnEndReached(self, handler):
		self.SetAction(handler, IE_ACT_CUSTOM)

class GSlider(GControl):
	pass

@add_metaclass(metaIDWrapper)
class GSaveGame:
	methods = {
		'GetDate': _GemRB.SaveGame_GetDate,
		'GetGameDate': _GemRB.SaveGame_GetGameDate,
		'GetName': _GemRB.SaveGame_GetName,
		'GetPortrait': _GemRB.SaveGame_GetPortrait,
		'GetPreview': _GemRB.SaveGame_GetPreview,
		'GetSaveID': _GemRB.SaveGame_GetSaveID,
	}

@add_metaclass(metaIDWrapper)
class GSprite2D:
	methods = {}
