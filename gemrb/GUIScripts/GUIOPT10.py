#autopause
import GemRB

def OnLoad():
	global AutoPauseWindow, TextAreaControl

	GemRB.LoadWindowPack("GUIOPT")
	AutoPauseWindow = GemRB.LoadWindow(10)
	TextAreaControl = GemRB.GetControl(AutoPauseWindow, 15)

	ChHitButton = GemRB.GetControl(AutoPauseWindow, 17)
	ChHitButtonB = GemRB.GetControl(AutoPauseWindow, 1)
	GemRB.SetVarAssoc(AutoPauseWindow, ChHitButtonB, "Autopause Status",1)

	ChInjured = GemRB.GetControl(AutoPauseWindow, 18)
	ChInjuredB = GemRB.GetControl(AutoPauseWindow, 2)
	GemRB.SetVarAssoc(AutoPauseWindow, ChInjuredB, "Autopause Status",2)

	ChDeath = GemRB.GetControl(AutoPauseWindow, 19)
	ChDeathB = GemRB.GetControl(AutoPauseWindow, 3)
	GemRB.SetVarAssoc(AutoPauseWindow, ChDeathB, "Autopause Status",4)

	ChAttacked = GemRB.GetControl(AutoPauseWindow, 20)
	ChAttackedB = GemRB.GetControl(AutoPauseWindow, 4)
	GemRB.SetVarAssoc(AutoPauseWindow, ChAttackedB, "Autopause Status",8)

	WeaponUnusable = GemRB.GetControl(AutoPauseWindow, 21)
	WeaponUnusableB = GemRB.GetControl(AutoPauseWindow, 5)
	GemRB.SetVarAssoc(AutoPauseWindow, WeaponUnusableB, "Autopause Status",16)

	TargetDestroyed = GemRB.GetControl(AutoPauseWindow, 22)
	TargetDestroyedB = GemRB.GetControl(AutoPauseWindow, 13)
	GemRB.SetVarAssoc(AutoPauseWindow, TargetDestroyedB, "Autopause Status",32)

	EndOfRound = GemRB.GetControl(AutoPauseWindow, 24)
	EndOfRoundB = GemRB.GetControl(AutoPauseWindow, 25)
	GemRB.SetVarAssoc(AutoPauseWindow, EndOfRoundB, "Autopause Status",64)

	EnemySighted = GemRB.GetControl(AutoPauseWindow, 27)
	EnemySightedB = GemRB.GetControl(AutoPauseWindow, 26)
	GemRB.SetVarAssoc(AutoPauseWindow, EnemySightedB, "Autopause Status",128)

	SpellCast = GemRB.GetControl(AutoPauseWindow, 30)
	SpellCastB = GemRB.GetControl(AutoPauseWindow, 34)
	GemRB.SetVarAssoc(AutoPauseWindow, SpellCastB, "Autopause Status",256)

	TrapFound = GemRB.GetControl(AutoPauseWindow, 33)
	TrapFoundB = GemRB.GetControl(AutoPauseWindow, 31)
	GemRB.SetVarAssoc(AutoPauseWindow, TrapFoundB, "Autopause Status",512)

	AutopauseCenter = GemRB.GetControl(AutoPauseWindow, 36)
	AutopauseCenterB = GemRB.GetControl(AutoPauseWindow, 37)
	GemRB.SetVarAssoc(AutoPauseWindow, AutopauseCenterB, "Autopause Center",1)

	OkButton = GemRB.GetControl(AutoPauseWindow, 11)
	CancelButton = GemRB.GetControl(AutoPauseWindow, 14)
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18044)
        GemRB.SetText(AutoPauseWindow, OkButton, 11973)
        GemRB.SetText(AutoPauseWindow, CancelButton, 13727)

	GemRB.SetEvent(AutoPauseWindow, ChHitButton, IE_GUI_BUTTON_ON_PRESS, "ChHitButtonPress")
	GemRB.SetButtonFlags(AutoPauseWindow, ChHitButtonB,IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetEvent(AutoPauseWindow, ChHitButtonB, IE_GUI_BUTTON_ON_PRESS, "ChHitButtonPress")

	GemRB.SetEvent(AutoPauseWindow, ChInjured, IE_GUI_BUTTON_ON_PRESS, "ChInjuredPress")
	GemRB.SetButtonFlags(AutoPauseWindow, ChInjuredB,IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetEvent(AutoPauseWindow, ChInjuredB, IE_GUI_BUTTON_ON_PRESS, "ChInjuredPress")

	GemRB.SetEvent(AutoPauseWindow, ChDeath, IE_GUI_BUTTON_ON_PRESS, "ChDeathPress")
	GemRB.SetButtonFlags(AutoPauseWindow, ChDeathB,IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetEvent(AutoPauseWindow, ChDeathB, IE_GUI_BUTTON_ON_PRESS, "ChDeathPress")

	GemRB.SetEvent(AutoPauseWindow, ChAttacked, IE_GUI_BUTTON_ON_PRESS, "ChAttackedPress")
	GemRB.SetButtonFlags(AutoPauseWindow, ChAttackedB,IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetEvent(AutoPauseWindow, ChAttackedB, IE_GUI_BUTTON_ON_PRESS, "ChAttackedPress")

	GemRB.SetEvent(AutoPauseWindow, WeaponUnusable, IE_GUI_BUTTON_ON_PRESS, "WeaponUnusablePress")
	GemRB.SetButtonFlags(AutoPauseWindow, WeaponUnusableB,IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetEvent(AutoPauseWindow, WeaponUnusableB, IE_GUI_BUTTON_ON_PRESS, "WeaponUnusablePress")

	GemRB.SetEvent(AutoPauseWindow, TargetDestroyed, IE_GUI_BUTTON_ON_PRESS, "TargetDestroyedPress")
	GemRB.SetButtonFlags(AutoPauseWindow, TargetDestroyedB,IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetEvent(AutoPauseWindow, TargetDestroyedB, IE_GUI_BUTTON_ON_PRESS, "TargetDestroyedPress")

	GemRB.SetEvent(AutoPauseWindow, EndOfRound, IE_GUI_BUTTON_ON_PRESS, "EndOfRoundPress")
	GemRB.SetButtonFlags(AutoPauseWindow, EndOfRoundB,IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetEvent(AutoPauseWindow, EndOfRoundB, IE_GUI_BUTTON_ON_PRESS, "EndOfRoundPress")

	GemRB.SetEvent(AutoPauseWindow, EnemySighted, IE_GUI_BUTTON_ON_PRESS, "EnemySightedPress")
	GemRB.SetButtonFlags(AutoPauseWindow, EnemySightedB,IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetEvent(AutoPauseWindow, EnemySightedB, IE_GUI_BUTTON_ON_PRESS, "EnemySightedPress")

	GemRB.SetEvent(AutoPauseWindow, SpellCast, IE_GUI_BUTTON_ON_PRESS, "SpellCastPress")
	GemRB.SetButtonFlags(AutoPauseWindow, SpellCastB,IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetEvent(AutoPauseWindow, SpellCastB, IE_GUI_BUTTON_ON_PRESS, "SpellCastPress")

	GemRB.SetEvent(AutoPauseWindow, TrapFound, IE_GUI_BUTTON_ON_PRESS, "TrapFoundPress")
	GemRB.SetButtonFlags(AutoPauseWindow, TrapFoundB,IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetEvent(AutoPauseWindow, TrapFoundB, IE_GUI_BUTTON_ON_PRESS, "TrapFoundPress")

	GemRB.SetEvent(AutoPauseWindow, AutopauseCenter, IE_GUI_BUTTON_ON_PRESS, "AutopauseCenterPress")
	GemRB.SetButtonFlags(AutoPauseWindow, AutopauseCenterB,IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetEvent(AutoPauseWindow, AutopauseCenterB, IE_GUI_BUTTON_ON_PRESS, "AutopauseCenterPress")

        GemRB.SetEvent(AutoPauseWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
        GemRB.SetEvent(AutoPauseWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
        GemRB.ShowModal(AutoPauseWindow)
	return

def ChHitButtonPress():
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18032)
	return

def ChInjuredPress():
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18033)
	return

def ChDeathPress():
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18034)
	return

def ChAttackedPress():
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18035)
	return

def WeaponUnusablePress():
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18036)
	return

def TargetDestroyedPress():
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18037)
	return

def EndOfRoundPress():
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 10640)
	return

def EnemySightedPress():
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 23514)
	return

def SpellCastPress():
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 58171)
	return

def TrapFoundPress():
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 31872)
	return

def AutopauseCenterPress():
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 10571)
	return

def OkPress():
        GemRB.SetVisible(AutoPauseWindow, 0)
        GemRB.UnloadWindow(AutoPauseWindow)
        GemRB.SetNextScript("GUIOPT8")
        return

def CancelPress():
        GemRB.SetVisible(AutoPauseWindow, 0)
        GemRB.UnloadWindow(AutoPauseWindow)
        GemRB.SetNextScript("GUIOPT8")
        return

