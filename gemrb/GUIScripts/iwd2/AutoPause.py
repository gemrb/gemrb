#autopause
import GemRB

def OnLoad():
	global AutoPauseWindow, TextAreaControl

	GemRB.LoadWindowPack("GUIOPT")
	
	AutoPauseWindow = GemRB.LoadWindow(10)
	TextAreaControl = GemRB.GetControl(AutoPauseWindow, 15)

	ChHitButton = GemRB.GetControl(AutoPauseWindow, 17)
	ChHitButtonB = GemRB.GetControl(AutoPauseWindow, 1)
	GemRB.SetButtonSprites(AutoPauseWindow, ChHitButtonB, "GBTNOPT4", 0, 0, 1, 2, 3)

	ChInjured = GemRB.GetControl(AutoPauseWindow, 18)
	ChInjuredB = GemRB.GetControl(AutoPauseWindow, 2)
	GemRB.SetButtonSprites(AutoPauseWindow, ChInjuredB, "GBTNOPT4", 0, 0, 1, 2, 3)

	ChDeath = GemRB.GetControl(AutoPauseWindow, 19)
	ChDeathB = GemRB.GetControl(AutoPauseWindow, 3)
	GemRB.SetButtonSprites(AutoPauseWindow, ChDeathB, "GBTNOPT4", 0, 0, 1, 2, 3)

	ChAttacked = GemRB.GetControl(AutoPauseWindow, 20)
	ChAttackedB = GemRB.GetControl(AutoPauseWindow, 4)
	GemRB.SetButtonSprites(AutoPauseWindow, ChAttackedB, "GBTNOPT4", 0, 0, 1, 2, 3)

	WeaponUnusable = GemRB.GetControl(AutoPauseWindow, 21)
	WeaponUnusableB = GemRB.GetControl(AutoPauseWindow, 5)
	GemRB.SetButtonSprites(AutoPauseWindow, WeaponUnusableB, "GBTNOPT4", 0, 0, 1, 2, 3)

	TargetDestroyed = GemRB.GetControl(AutoPauseWindow, 22)
	TargetDestroyedB = GemRB.GetControl(AutoPauseWindow, 13)
	GemRB.SetButtonSprites(AutoPauseWindow, TargetDestroyedB, "GBTNOPT4", 0, 0, 1, 2, 3)

	EndOfRound = GemRB.GetControl(AutoPauseWindow, 24)
	EndOfRoundB = GemRB.GetControl(AutoPauseWindow, 25)
	GemRB.SetButtonSprites(AutoPauseWindow, EndOfRoundB, "GBTNOPT4", 0, 0, 1, 2, 3)

	EnemySighted = GemRB.GetControl(AutoPauseWindow, 31)
	EnemySightedB = GemRB.GetControl(AutoPauseWindow, 30)
	GemRB.SetButtonSprites(AutoPauseWindow, EnemySightedB, "GBTNOPT4", 0, 0, 1, 2, 3)

	SpellCast = GemRB.GetControl(AutoPauseWindow, 37)
	SpellCastB = GemRB.GetControl(AutoPauseWindow, 36)
	GemRB.SetButtonSprites(AutoPauseWindow, SpellCastB, "GBTNOPT4", 0, 0, 1, 2, 3)

	TrapFound = GemRB.GetControl(AutoPauseWindow, 28)
	TrapFoundB = GemRB.GetControl(AutoPauseWindow, 26)
	GemRB.SetButtonSprites(AutoPauseWindow, TrapFoundB, "GBTNOPT4", 0, 0, 1, 2, 3)

	AutopauseCenter = GemRB.GetControl(AutoPauseWindow, 34)
	AutopauseCenterB = GemRB.GetControl(AutoPauseWindow, 33)
	GemRB.SetButtonSprites(AutoPauseWindow, AutopauseCenterB, "GBTNOPT4", 0, 0, 1, 2, 3)

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

	GemRB.SetVarAssoc(AutoPauseWindow, AutopauseCenterB, "Auto Pause Center",1)

	GemRB.SetVarAssoc(AutoPauseWindow, ChHitButtonB, "Auto Pause Status",1)
	GemRB.SetVarAssoc(AutoPauseWindow, ChInjuredB, "Auto Pause Status",2)
	GemRB.SetVarAssoc(AutoPauseWindow, ChDeathB, "Auto Pause Status",4)
	GemRB.SetVarAssoc(AutoPauseWindow, ChAttackedB, "Auto Pause Status",8)
	GemRB.SetVarAssoc(AutoPauseWindow, WeaponUnusableB, "Auto Pause Status",16)
	GemRB.SetVarAssoc(AutoPauseWindow, TargetDestroyedB, "Auto Pause Status",32)
	GemRB.SetVarAssoc(AutoPauseWindow, EndOfRoundB, "Auto Pause Status",64)
	GemRB.SetVarAssoc(AutoPauseWindow, EnemySightedB, "Auto Pause Status",128)
	GemRB.SetVarAssoc(AutoPauseWindow, SpellCastB, "Auto Pause Status",256)
	GemRB.SetVarAssoc(AutoPauseWindow, TrapFoundB, "Auto Pause Status",512)

        GemRB.SetVisible(AutoPauseWindow,1)
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
        GemRB.UnloadWindow(AutoPauseWindow)
        GemRB.SetNextScript("GamePlay")
        return

def CancelPress():
        GemRB.UnloadWindow(AutoPauseWindow)
        GemRB.SetNextScript("GamePlay")
        return
