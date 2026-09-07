# SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

"""Behavioral GUI regressions using synthetic tables and a controlled engine API.

These tests execute the real selection functions without requiring game assets
or starting the engine. They do not constitute live gameplay acceptance.
"""

from contextlib import redirect_stdout
import importlib.util
import io
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch


SCRIPTS = Path(__file__).resolve().parents[2] / "GUIScripts"


def load_script(relative, modules=None):
    path = SCRIPTS / relative
    spec = importlib.util.spec_from_file_location(path.stem, path)
    module = importlib.util.module_from_spec(spec)
    with patch.dict(sys.modules, modules or {}):
        spec.loader.exec_module(module)
    return module


DEFINES = load_script("GUIDefines.py")
STATS = load_script("ie_stats.py")
SPELLS = load_script("ie_spells.py")


class Table:
    def __init__(self, columns, rows):
        self.columns = columns
        self.rows = rows

    def GetValue(self, row, column, *unused):
        if isinstance(row, str):
            row = self.GetRowIndex(row)
        if isinstance(column, str):
            column = self.columns.index(column)
        return self.rows[row][1][column]

    def GetRowCount(self):
        return len(self.rows)

    def GetRowName(self, index):
        return self.rows[index][0]

    def GetRowIndex(self, name):
        return [row[0] for row in self.rows].index(name)

    def GetColumnCount(self):
        return len(self.columns)

    def GetColumnName(self, index):
        return self.columns[index]


class Control:
    API = {
        "OnPress", "OnChange", "SetVarAssoc", "SetActionInterval", "SetState",
        "SetFlags", "SetDisabled", "MakeDefault", "SetTooltip", "SetSpellIcon",
        "SetSprites", "SetBorder",
    }

    def __init__(self, engine):
        self.engine = engine
        self.texts = []
        self.calls = []

    def SetText(self, text):
        # Model token expansion at the exact SetText call, not when asserted.
        self.texts.append((text, dict(self.engine.tokens)))

    def __getattr__(self, name):
        if name not in self.API:
            raise AttributeError(name)
        return lambda *args: self.calls.append((name, args))


class Window:
    def __init__(self, engine, native_ids=None):
        self.engine = engine
        self.strict = native_ids is not None
        self.controls = {key: Control(engine) for key in native_ids or ()}
        self.aliases = {}
        self.created_buttons = []
        self.modal = None

    def GetControl(self, control_id):
        if control_id not in self.controls and not self.strict:
            self.controls[control_id] = Control(self.engine)
        return self.controls.get(control_id)

    def AliasControls(self, mapping):
        self.aliases.update(mapping)

    def GetControlAlias(self, alias):
        return self.GetControl(self.aliases[alias])

    def SetEventProxy(self, control):
        self.event_proxy = control

    def CreateButton(self, control_id, *unused):
        if control_id in self.controls:
            raise AssertionError("native control must not be replaced")
        self.created_buttons.append(control_id)
        self.controls[control_id] = Control(self.engine)
        return self.controls[control_id]

    def CreateScrollBar(self, control_id, *unused):
        self.controls[control_id] = Control(self.engine)
        return self.controls[control_id]

    def ShowModal(self, shadow):
        self.modal = shadow


class Engine:
    def __init__(self, tables):
        self.tables = tables
        self.loaded = []
        self.variables = {}
        self.tokens = {"number": "2"}  # left over from the preceding spell chooser
        self.stats = {STATS.IE_CLASS: 22, STATS.IE_ALIGNMENT: 18}

    def LoadTable(self, name, *unused):
        self.loaded.append(name)
        return self.tables[name]

    def SetVar(self, name, value):
        self.variables[name] = value

    def GetVar(self, name):
        return self.variables.get(name, 0)

    def SetToken(self, name, value):
        self.tokens[name] = value

    def GetPlayerStat(self, pc, stat, *unused):
        return self.stats.get(stat, 0)

    def CountEffects(self, *unused):
        return 0

    def LoadWindow(self, *unused):
        return self.window

    def GetSpell(self, resource):
        return {"SpellName": resource, "SpellDesc": resource}


def dependencies(engine, common, ee=True):
    return {
        "GemRB": engine, "GUIDefines": DEFINES, "ie_stats": STATS, "ie_spells": SPELLS,
        "GameCheck": SimpleNamespace(IsBG2OrEE=lambda: True, IsPST=lambda: False,
                                     IsAnyEE=lambda: ee, IsBG2EE=lambda: ee),
        "GUICommon": common, "CommonTables": SimpleNamespace(),
        "CommonWindow": SimpleNamespace(AddScrollbarProxy=lambda *args: None),
        "Spellbook": SimpleNamespace(),
    }


class SkillSelectionTests(unittest.TestCase):
    def setup_selection(self, points=20, skill_rows=True):
        names = {22: "SORCERER_MONK", 19: "SORCERER", 20: "MONK"}
        skills = [("STEALTH", [100, 12000, 12001, 1])] if skill_rows else []
        engine = Engine({
            "skills": Table(["ID", "CAP_REF", "DESC_REF", "SORCERER_MONK"], skills),
            "thiefscl": Table(["SORCERER_MONK"], [("STEALTH", [1])]),
            "thiefskl": Table(["START_POINTS", "LEVEL_POINTS"], [("SORCERER_MONK", [points, 10])]),
        })
        common = SimpleNamespace(
            IsDualClassedDetailed=lambda pc: (0, 0, 0), IsMultiClassed=lambda *args: (2, 19, 20),
            GetClassRowName=lambda value, *args: names[value], IsNamelessOne=lambda pc: False,
            GetKitIndex=lambda pc: 0,
        )
        modules = dependencies(engine, common)
        modules["CommonTables"].ClassSkills = Table(
            ["THIEFSKILL"], [("SORCERER", ["*"]), ("MONK", ["thiefskl"])])
        module = load_script("LUSkillsSelection.py", modules)
        window = Window(engine)
        return engine, module, window

    def test_chargen_and_dualclass_expand_actual_component_skill_points(self):
        for skilltype, text_control in ((2, 19), (4, 22)):
            with self.subTest(skilltype=skilltype):
                engine, module, window = self.setup_selection()
                module.SetupSkillsWindow(1, skilltype, window, lambda: None,
                                         level1=[0, 0], level2=[1, 2])
                self.assertEqual(engine.GetVar("SkillPointsLeft"), 30)
                intros = [tokens for text, tokens in window.GetControl(text_control).texts if text == 17248]
                self.assertEqual(intros, [{"number": "30"}])

    def test_levelup_does_not_replace_description_with_chargen_intro(self):
        engine, module, window = self.setup_selection()
        module.SetupSkillsWindow(1, module.LUSKILLS_TYPE_LEVELUP, window, lambda: None,
                                 level1=[1, 1], level2=[1, 2])
        self.assertEqual(engine.GetVar("SkillPointsLeft"), 10)
        self.assertNotIn(17248, [text for text, _ in window.GetControl(110).texts])
        self.assertEqual(engine.tokens["number"], "2")

    def test_empty_and_zero_point_screens_do_not_expand_stale_intro(self):
        for points, skill_rows in ((0, True), (20, False)):
            with self.subTest(points=points, skill_rows=skill_rows):
                engine, module, window = self.setup_selection(points, skill_rows)
                module.SetupSkillsWindow(1, module.LUSKILLS_TYPE_CHARGEN, window, lambda: None,
                                         level1=[0, 0], level2=[1, 1])
                self.assertNotIn(17248, [text for text, _ in window.GetControl(19).texts])
                self.assertEqual(engine.tokens["number"], "2")


class HLASelectionTests(unittest.TestCase):
    def setup_selection(self, count=18, minimum=1, maximum=99, merged=True, ee=True):
        row_name = "SORCERER_MONK" if merged else "FIGHTER"
        engine = Engine({
            "luabbr": Table(["ABBREV"], [(row_name, ["test"])]),
            "lutest": Table(list(range(9)), [
                (str(i), [f"GA_TEST{i:03d}", "*", "*", minimum, maximum, 1, "*", "*", "*"])
                for i in range(count)
            ]),
        })
        common = SimpleNamespace(
            GetKitIndex=lambda pc: 0, IsDualClassed=lambda pc: False,
            GetClassRowName=lambda value, *args: {19: "SORCERER", 20: "MONK", 2: "FIGHTER"}[value],
            GetKitRowName=lambda *args: row_name, ceildiv=lambda a, b: -(-a // b),
        )
        module = load_script("bg2/LUHLASelection.py", dependencies(engine, common, ee))
        module.pc = 1
        module.NumClasses = 2 if merged else 1
        module.Classes = [19, 20] if merged else [2]
        module.Level = [12, 20] if merged else [20]
        module.HLACount = count + 1
        return engine, module

    def get_hlas(self, module):
        with redirect_stdout(io.StringIO()):
            module.GetHLAs()

    def test_merged_table_has_one_row_and_selection_credit_per_ability(self):
        engine, module = self.setup_selection()
        self.get_hlas(module)
        self.assertEqual(engine.loaded.count("lutest"), 1)
        self.assertEqual(len(module.HLAAbilities), 18)
        self.assertEqual(len({row[0] for row in module.HLAAbilities}), 18)
        self.assertEqual(module.HLANewAbilities, [0] * 18)
        self.assertEqual(module.HLACount, 18)
        self.assertEqual(engine.GetVar("HLACount"), 18)

    def test_higher_active_component_can_unlock_merged_ability(self):
        for levels in ([12, 20], [20, 12]):
            with self.subTest(levels=levels):
                _, module = self.setup_selection(count=1, minimum=18)
                module.Level = levels
                self.get_hlas(module)
                self.assertEqual([row[1] for row in module.HLAAbilities], [1])

    def test_merged_level_bounds_preserve_union_of_active_components(self):
        for levels, minimum, maximum, eligible in (
            ([18, 20], 1, 19, 1),
            ([20, 18], 18, 19, 1),
            ([12, 20], 18, 19, 0),
            ([12, 16, 20], 18, 99, 0),  # third entry is not an active component
        ):
            with self.subTest(levels=levels, minimum=minimum, maximum=maximum):
                _, module = self.setup_selection(count=1, minimum=minimum, maximum=maximum)
                module.Level = levels
                self.get_hlas(module)
                self.assertEqual([row[1] for row in module.HLAAbilities], [eligible])
                self.assertEqual(module.HLACount, eligible)

    def test_level_bounds_and_single_class_remain_enforced(self):
        for minimum, maximum, eligible in ((1, 99, 1), (21, 99, 0), (1, 19, 0)):
            with self.subTest(minimum=minimum, maximum=maximum):
                engine, module = self.setup_selection(count=1, minimum=minimum,
                                                       maximum=maximum, merged=False)
                self.get_hlas(module)
                self.assertEqual([row[1] for row in module.HLAAbilities], [eligible])
                self.assertEqual(module.HLACount, eligible)
                self.assertEqual(engine.loaded.count("lutest"), 1)

    def test_native_ee_extra_button_is_reused_and_classic_button_is_created(self):
        for ee in (True, False):
            for count in (25, 26):
                with self.subTest(ee=ee, count=count):
                    engine, module = self.setup_selection(count=count, ee=ee)
                    ids = list(range(25 if ee else 24)) + [26, 28, 41, 42, 0x10000017, 0x10000018]
                    engine.window = Window(engine, ids)
                    original = engine.window.GetControl(24)
                    engine.SetVar("HLACount", 1)
                    with redirect_stdout(io.StringIO()):
                        module.OpenHLAWindow(1, 2, [19, 20], [12, 20])
                    self.assertEqual(engine.window.created_buttons, [] if ee else [24])
                    if ee:
                        self.assertIs(engine.window.GetControl(24), original)
                    self.assertIsNotNone(engine.window.modal)
                    self.assertEqual(1000 in engine.window.controls, count > 25)


if __name__ == "__main__":
    unittest.main()
