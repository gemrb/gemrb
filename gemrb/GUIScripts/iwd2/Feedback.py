#feedback
import GemRB

FeedbackWindow = 0
TextAreaControl = 0

def OnLoad():
	global FeedbackWindow, TextAreaControl

	GemRB.LoadWindowPack("GUIOPT")
	FeedbackWindow = GemRB.LoadWindow(9)

	MarkerSlider = GemRB.GetControl(FeedbackWindow, 30)
	MarkerSliderS = GemRB.GetControl(FeedbackWindow, 8)

	LocatorSlider = GemRB.GetControl(FeedbackWindow, 31)
	LocatorSliderS = GemRB.GetControl(FeedbackWindow, 9)

	THac0Rolls = GemRB.GetControl(FeedbackWindow, 32)
	THac0RollsB = GemRB.GetControl(FeedbackWindow, 10)
	GemRB.SetButtonSprites(FeedbackWindow, THac0RollsB, "GBTNOPT4", 0, 0, 1, 2, 3)

	CombatInfo = GemRB.GetControl(FeedbackWindow, 33)
	CombatInfoB = GemRB.GetControl(FeedbackWindow, 11)
	GemRB.SetButtonSprites(FeedbackWindow, CombatInfoB, "GBTNOPT4", 0, 0, 1, 2, 3)

	Actions = GemRB.GetControl(FeedbackWindow, 34)
	ActionsB = GemRB.GetControl(FeedbackWindow, 12)
	GemRB.SetButtonSprites(FeedbackWindow, ActionsB, "GBTNOPT4", 0, 0, 1, 2, 3)

	StateChanges = GemRB.GetControl(FeedbackWindow, 35)
	StateChangesB = GemRB.GetControl(FeedbackWindow, 13)
	GemRB.SetButtonSprites(FeedbackWindow, StateChangesB, "GBTNOPT4", 0, 0, 1, 2, 3)

	SelectionText = GemRB.GetControl(FeedbackWindow, 36)
	SelectionTextB = GemRB.GetControl(FeedbackWindow, 14)
	GemRB.SetButtonSprites(FeedbackWindow, SelectionTextB, "GBTNOPT4", 0, 0, 1, 2, 3)

	Miscellaneous = GemRB.GetControl(FeedbackWindow, 37)
	MiscellaneousB = GemRB.GetControl(FeedbackWindow, 15)
	GemRB.SetButtonSprites(FeedbackWindow, MiscellaneousB, "GBTNOPT4", 0, 0, 1, 2, 3)
	
	OkButton = GemRB.GetControl(FeedbackWindow, 26)
	CancelButton = GemRB.GetControl(FeedbackWindow, 27)
	TextAreaControl = GemRB.GetControl(FeedbackWindow, 28)

        GemRB.SetText(FeedbackWindow, TextAreaControl, 18043)
        GemRB.SetText(FeedbackWindow, OkButton, 11973)
        GemRB.SetText(FeedbackWindow, CancelButton, 13727)
	
	GemRB.SetEvent(FeedbackWindow, MarkerSlider, IE_GUI_BUTTON_ON_PRESS, "MarkerSliderPress")
	GemRB.SetEvent(FeedbackWindow, MarkerSliderS, IE_GUI_SLIDER_ON_CHANGE, "MarkerSliderPress")
	GemRB.SetVarAssoc(FeedbackWindow, MarkerSliderS, "GUI Feedback Level",1)

	GemRB.SetEvent(FeedbackWindow, LocatorSlider, IE_GUI_BUTTON_ON_PRESS, "LocatorSliderPress")
	GemRB.SetEvent(FeedbackWindow, LocatorSliderS, IE_GUI_SLIDER_ON_CHANGE, "LocatorSliderPress")
	GemRB.SetVarAssoc(FeedbackWindow, LocatorSliderS, "Locator Feedback Level",1)

	GemRB.SetEvent(FeedbackWindow, THac0Rolls, IE_GUI_BUTTON_ON_PRESS, "THac0RollsPress")
	GemRB.SetEvent(FeedbackWindow, THac0RollsB, IE_GUI_BUTTON_ON_PRESS, "THac0RollsBPress")
	GemRB.SetButtonFlags(FeedbackWindow,THac0RollsB, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetVarAssoc(FeedbackWindow,THac0RollsB, "Rolls",1)

	GemRB.SetEvent(FeedbackWindow, CombatInfo, IE_GUI_BUTTON_ON_PRESS, "CombatInfoPress")
	GemRB.SetEvent(FeedbackWindow, CombatInfoB, IE_GUI_BUTTON_ON_PRESS, "CombatInfoBPress")
	GemRB.SetButtonFlags(FeedbackWindow,CombatInfoB, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetVarAssoc(FeedbackWindow,CombatInfoB, "Combat Info",1)

	GemRB.SetEvent(FeedbackWindow, Actions, IE_GUI_BUTTON_ON_PRESS, "ActionsPress")
	GemRB.SetEvent(FeedbackWindow, ActionsB, IE_GUI_BUTTON_ON_PRESS, "ActionsBPress")
	GemRB.SetButtonFlags(FeedbackWindow,ActionsB, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetVarAssoc(FeedbackWindow,ActionsB, "Actions",1)

	GemRB.SetEvent(FeedbackWindow, StateChanges, IE_GUI_BUTTON_ON_PRESS, "StateChangesPress")
	GemRB.SetEvent(FeedbackWindow, StateChangesB, IE_GUI_BUTTON_ON_PRESS, "StateChangesBPress")
	GemRB.SetButtonFlags(FeedbackWindow,StateChangesB, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetVarAssoc(FeedbackWindow,StateChangesB, "State Changes",1)

	GemRB.SetEvent(FeedbackWindow, SelectionText, IE_GUI_BUTTON_ON_PRESS, "SelectionTextPress")
	GemRB.SetEvent(FeedbackWindow, SelectionTextB, IE_GUI_BUTTON_ON_PRESS, "SelectionTextBPress")
	GemRB.SetButtonFlags(FeedbackWindow,SelectionTextB, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetVarAssoc(FeedbackWindow,SelectionTextB, "Selection Text",1)

	GemRB.SetEvent(FeedbackWindow, Miscellaneous, IE_GUI_BUTTON_ON_PRESS, "MiscellaneousPress")
	GemRB.SetEvent(FeedbackWindow, MiscellaneousB, IE_GUI_BUTTON_ON_PRESS, "MiscellaneousBPress")
	GemRB.SetButtonFlags(FeedbackWindow,MiscellaneousB, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetVarAssoc(FeedbackWindow,MiscellaneousB, "Miscellaneous Text",1)

        GemRB.SetEvent(FeedbackWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
        GemRB.SetEvent(FeedbackWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
        GemRB.SetVisible(FeedbackWindow,1)
	return

def MarkerSliderPress():
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18024)
	return

def LocatorSliderPress():
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18025)
	return

def THac0RollsPress():
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18026)
	return

def CombatInfoPress():
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18027)
	return

def ActionsPress():
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18028)
	return

def StateChangesPress():
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18029)
	return

def SelectionTextPress():
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18030)
	return

def MiscellaneousPress():
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18031)
	return

def OkPress():
        GemRB.UnloadWindow(FeedbackWindow)
        GemRB.SetNextScript("GamePlay")
        return

def CancelPress():
        GemRB.UnloadWindow(FeedbackWindow)
        GemRB.SetNextScript("GamePlay")
        return

