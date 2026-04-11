# SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# code shared between the common GUICG12, CGPortrait
import GemRB
import GameCheck
from GUIDefines import *

# This if for moving to next portrait
def portrait_next(PortraitsTable, LastPortrait, gender):
    print ("portrait_next")
    while True:
        LastPortrait += 1
        if LastPortrait >= PortraitsTable.GetRowCount():
            LastPortrait = 0

        if PortraitsTable.GetValue(LastPortrait, 0) == gender:
            return LastPortrait

# This is for moving to previous portrait
def portrait_prev(PortraitsTable, LastPortrait, gender):
    print ("portrait_prev")
    while True:
        LastPortrait -= 1
        if LastPortrait < 0:
            LastPortrait = PortraitsTable.GetRowCount() - 1

        if PortraitsTable.GetValue(LastPortrait, 0) == gender:
            return LastPortrait

# This is for when is done
def portrait_custom_done(custom_window, appearance_window, portrait_list1, portrait_list2):
    print ("portrait_custom_done")
    portrait = portrait_list1.QueryText()
    GemRB.SetToken("LargePortrait", portrait)

    portrait = portrait_list2.QueryText()
    GemRB.SetToken("SmallPortrait", portrait)

    if custom_window:
        custom_window.Close()

    if appearance_window:
        appearance_window.Close()
    if GameCheck.IsBG1OrEE():
        CharGenCommon.next()
    elif GameCheck.IsIWD2():
        GemRB.SetNextScript ("CharGen2")

# This is for abort
def portrait_custom_abort(custom_window):
    print ("portrait_custom_abort")
    if custom_window:
        custom_window.Close()
    if GameCheck.IsBG1OrEE():
        AppearanceWindow.ShowModal (MODAL_SHADOW_GRAY) # narrower than CustomWindow, so borders will remain
    elif GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
        AppearanceWindow.ShowModal (MODAL_SHADOW_NONE) # narrower than CustomWindow, so borders will remain

# This is for applying
def portrait_apply_selection(AppearanceWindow, LastPortrait):
    print ("portrait_apply_selection")
    if AppearanceWindow:
        AppearanceWindow.Close ()
    PortraitTable = GemRB.LoadTable("pictures")
    PortraitName = PortraitTable.GetRowName(LastPortrait)
    large_suffix = "L"
    if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
        large_suffix = "M"
    GemRB.SetToken("SmallPortrait", PortraitName + "S")
    GemRB.SetToken("LargePortrait", PortraitName + large_suffix)
    if GameCheck.IsBG1OrEE():
        CharGenCommon.next()
    elif GameCheck.IsIWD2():
        GemRB.SetNextScript ("CharGen2") #Before race
    elif GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
        GemRB.SetNextScript ("CharGen2") #Before race

# This is for the large custom portrait
def portrait_common_large_custom(
    custom_window,
    portrait_list1,
    portrait_list2,
    row_count1,
    empty_portrait
):
    print ("portrait_common_large_custom")
    window = custom_window

    portrait = portrait_list1.QueryText()

    # small hack
    if GemRB.GetVar("Row1") == row_count1:
        return

    label = window.GetControl(0x10000007)
    label.SetText(portrait)

    button = window.GetControl(6)

    if portrait == "":
        portrait = empty_portrait
        if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
            button.SetDisabled(True)
        else:
            button.SetState(IE_GUI_BUTTON_DISABLED)
    else:
        if portrait_list2.QueryText() != "":
            if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
                button.SetDisabled(False)
            else:
                button.SetState(IE_GUI_BUTTON_ENABLED)

    preview = window.GetControl(0)
    preview.SetPicture(portrait, empty_portrait)

# This is for the small custom portrait
def portrait_common_small_custom(
    custom_window,
    portrait_list1,
    portrait_list2,
    row_count2
):
    print ("portrait_common_small_custom")
    window = custom_window

    portrait = portrait_list2.QueryText()

    # small hack
    if GemRB.GetVar("Row2") == row_count2:
        return

    label = window.GetControl(0x10000008)
    label.SetText(portrait)

    button = window.GetControl(6)

    if portrait == "":
        portrait = "NOPORTSM"

        if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
            button.SetDisabled(True)
        else:
            button.SetState(IE_GUI_BUTTON_DISABLED)
    else:
        if portrait_list1.QueryText() != "":
            if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
                button.SetDisabled(False)
            else:
                button.SetState(IE_GUI_BUTTON_ENABLED)

    preview = window.GetControl(1)
    preview.SetPicture(portrait, "NOPORTSM")


def portrait_custom_press(
    portraits_table,
    last_portrait,
    large_custom_handler,
    small_custom_handler,
    custom_done,
    custom_abort
):
    print ("portrait_custom_press")
    global PortraitList1, PortraitList2
    global RowCount1, RowCount2
    global CustomWindow

    window_pack=None
    if not GameCheck.IsIWD2():
        window_pack="GUICG"
    if window_pack:
        CustomWindow = Window = GemRB.LoadWindow(18, window_pack)
    else:
        CustomWindow = Window = GemRB.LoadWindow(18)

    list_mode1=1
    if GameCheck.IsIWD2():
        list_mode1=2
    PortraitList1 = Window.GetControl(2)
    RowCount1 = len(PortraitList1.ListResources(CHR_PORTRAITS, list_mode1))
    PortraitList1.OnSelect(large_custom_handler)
    PortraitList1.SetVarAssoc("Row1", RowCount1)

    PortraitList2 = Window.GetControl(4)
    RowCount2 = len(PortraitList2.ListResources(CHR_PORTRAITS, 0))
    PortraitList2.OnSelect(small_custom_handler)
    PortraitList2.SetVarAssoc("Row2", RowCount2)

    Button = Window.GetControl(6)
    Button.SetText(11973)
    if GameCheck.IsIWD2():
        Button.MakeDefault()
    Button.OnPress(custom_done)
    if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
        Button.SetDisabled(True)
    else:
        Button.SetState(IE_GUI_BUTTON_DISABLED)

    Button = Window.GetControl(7)
    Button.SetText(15416)
    if GameCheck.IsIWD2():
        Button.MakeEscape()
    Button.OnPress(custom_abort)

    large_suffix="L"
    if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
        large_suffix="M"
    fallback_resource="NOPORTMD"
    if GameCheck.IsBG1OrEE():
        fallback_resource="NOPORTLG"
    Button = Window.GetControl(0)
    portrait_name = portraits_table.GetRowName(last_portrait) + large_suffix

    if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
        if GemRB.HasResource(portrait_name, RES_BMP, 1) or GemRB.HasResource(fallback_resource, RES_BMP, 1):
            Button.SetPicture(portrait_name, fallback_resource)
    else:
        Button.SetPicture(portrait_name, fallback_resource)

    Button.SetState(IE_GUI_BUTTON_LOCKED)

    Button = Window.GetControl(1)
    portrait_name = portraits_table.GetRowName(last_portrait) + "S"
    Button.SetPicture(portrait_name, "NOPORTSM")
    Button.SetState(IE_GUI_BUTTON_LOCKED)

    modal_shadow = MODAL_SHADOW_NONE
    if GameCheck.IsBG1OrEE():
        modal_shadow = MODAL_SHADOW_GRAY
    Window.ShowModal(modal_shadow)

def portrait_back_press(AppearanceWindow):
    print ("portrait_back_press")
    if AppearanceWindow:
        AppearanceWindow.Close ()
    next_script = "GUICG1"
    if GameCheck.IsIWD2():
        next_script = "CharGen"
    GemRB.SetNextScript (next_script)
    GemRB.SetVar ("Gender",0) #scrapping the gender value
    return

def portrait_set_picture(PortraitButton, PortraitsTable, LastPortrait):
    print ("portrait_set_picture")
    picture_size = "L"
    if GameCheck.IsBG1OrEE():
        picture_size = "G"
    PortraitName = PortraitsTable.GetRowName (LastPortrait)+picture_size
    if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
        PortraitButton.SetPicture (PortraitName, "NOPORTLG")
    else:
        PortraitButton.SetPicture (PortraitName)
    return