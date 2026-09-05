# SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

"""Exercise actual portrait callbacks without game assets or an engine process."""

from contextlib import contextmanager
import importlib.util
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch


SCRIPTS = Path(__file__).resolve().parents[2] / "GUIScripts"


def load_script(name):
    path = SCRIPTS / (name + ".py")
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class Control:
    def __init__(self):
        self.callback = None

    def OnPress(self, callback):
        self.callback = callback

    def SetText(self, text):
        self.text = text

    def SetFlags(self, flags, operation):
        self.flags = flags

    def SetState(self, state):
        self.state = state

    def MakeEscape(self):
        self.escape = True

    def MakeDefault(self):
        self.default = True

    def SetPicture(self, resource, fallback):
        self.picture = resource


class Window:
    def __init__(self):
        self.controls = {index: Control() for index in range(8)}
        self.closed = False

    def GetControl(self, index):
        return self.controls[index]

    def Close(self):
        self.closed = True

    def Focus(self):
        self.focused = True

    def ShowModal(self, shadow):
        self.modal = shadow


class Pictures:
    names = ("MAN1", "MAN2", "WOMAN1", "WOMAN2")

    def FindValue(self, column, value):
        assert (column, value) == (0, 2)
        return 2

    def GetRowCount(self):
        return len(self.names)

    def GetRowName(self, index):
        return self.names[index]


class Engine:
    def __init__(self, game):
        self.GameType = game
        self.variables = {"Slot": 1, "Gender": 1, "MaxPartySize": 6}
        self.tokens = {}
        self.scripts = []
        self.filled_portraits = []
        self.window = Window()

    def GetVar(self, name):
        return self.variables[name]

    def SetVar(self, name, value):
        self.variables[name] = value

    def SetToken(self, name, value):
        self.tokens[name] = value

    def SetNextScript(self, name):
        self.scripts.append(name)

    def GetPlayerStat(self, pc, stat):
        return 1

    def LoadWindow(self, window_id, pack):
        assert (window_id, pack) in ((11, "GUICG"), (18, "GUIREC"))
        return self.window

    def LoadTable(self, name):
        assert name == "PICTURES"
        return Pictures()

    def Roll(self, count, sides, bonus):
        assert count == 1 and sides > 0
        return 1 + bonus

    def GameGetSelectedPCSingle(self):
        return 1

    def GetPlayerPortrait(self, pc, size):
        return {"ResRef": "MAN2m"}

    def FillPlayerInfo(self, pc, large, small):
        self.filled_portraits.append((pc, large, small))


@contextmanager
def portrait_session(game):
    engine = Engine(game)
    callbacks = []
    if game == "bg1":
        chargen = SimpleNamespace(
            back=lambda window: callbacks.append(("back", window)),
            next=lambda: callbacks.append(("next",)),
        )
    else:
        # The bg2 CharGenCommon API deliberately has no BG1 back/next helpers.
        chargen = SimpleNamespace(PositionCharGenWin=lambda window, offset: callbacks.append(("position", window, offset)))
    modules = {
        "GemRB": engine, "CharGenCommon": chargen,
        "GUIDefines": load_script("GUIDefines"),
        "ie_restype": load_script("ie_restype"), "ie_stats": load_script("ie_stats"),
    }
    with patch.dict(sys.modules, modules):
        # Exercise the actual family predicates rather than duplicating them.
        with patch.dict(sys.modules, {"GameCheck": load_script("GameCheck")}):
            yield engine, load_script("GUIPortraitCommon"), callbacks


class PortraitNavigationTests(unittest.TestCase):
    def test_bg1_back_uses_legacy_chargen_callback(self):
        with portrait_session("bg1") as (engine, portrait, callbacks):
            portrait.OnLoad()
            engine.window.GetControl(5).callback()
            self.assertEqual(callbacks, [("back", engine.window)])
            self.assertEqual(engine.scripts, [])

    def test_bgee_and_bg2_back_return_to_gender_without_bg1_api(self):
        for game in ("bgee", "bg2", "bg2ee"):
            with self.subTest(game=game), portrait_session(game) as (engine, portrait, callbacks):
                portrait.OnLoad()
                engine.window.GetControl(5).callback()
                self.assertEqual(callbacks, [("position", engine.window, -6)])
                self.assertTrue(engine.window.closed)
                self.assertEqual(engine.scripts, ["GUICG1"])
                self.assertEqual(engine.GetVar("Gender"), 0)

    def test_builtin_done_preserves_each_family_next_route(self):
        for game in ("bg1", "bgee", "bg2", "bg2ee"):
            with self.subTest(game=game), portrait_session(game) as (engine, portrait, callbacks):
                portrait.OnLoad()
                engine.window.GetControl(0).callback()
                self.assertTrue(engine.window.closed)
                self.assertEqual(engine.tokens["SmallPortrait"], "MAN2S")
                self.assertEqual(engine.tokens["LargePortrait"], "MAN2L" if game == "bg1" else "MAN2M")
                self.assertEqual(engine.scripts, [] if game == "bg1" else ["CharGen2"])
                self.assertEqual([call[0] for call in callbacks], ["next"] if game == "bg1" else ["position"])

    def test_custom_done_preserves_each_family_next_route(self):
        for game in ("bg1", "bgee", "bg2", "bg2ee"):
            with self.subTest(game=game), portrait_session(game) as (engine, portrait, callbacks):
                portrait.OnLoad()
                portrait.CustomWindow = Window()
                portrait.PortraitList1 = SimpleNamespace(QueryText=lambda: "CUSTOML")
                portrait.PortraitList2 = SimpleNamespace(QueryText=lambda: "CUSTOMS")
                portrait.PortraitButtonCustomDone()
                self.assertTrue(portrait.CustomWindow.closed)
                self.assertTrue(engine.window.closed)
                self.assertEqual(engine.tokens, {"LargePortrait": "CUSTOML", "SmallPortrait": "CUSTOMS"})
                self.assertEqual(engine.scripts, [] if game == "bg1" else ["CharGen2"])
                self.assertEqual([call[0] for call in callbacks], ["next"] if game == "bg1" else ["position"])

    def test_record_portrait_cancel_does_not_navigate_chargen(self):
        for game in ("bg1", "bgee", "bg2", "bg2ee"):
            with self.subTest(game=game), portrait_session(game) as (engine, portrait, callbacks):
                portrait.OnLoad(PortraitModification=True)
                engine.window.GetControl(4).callback()
                self.assertTrue(engine.window.closed)
                self.assertEqual(callbacks, [])
                self.assertEqual(engine.scripts, [])


if __name__ == "__main__":
    unittest.main()
