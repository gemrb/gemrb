#!/usr/bin/env python3
"""Native BGEE level-up routing using actual GameCheck and GUI callbacks."""

import ast
from contextlib import redirect_stdout
import io
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import test_levelup_selection as support


class Table(support.Table):
    def GetColumnIndex(self, name):
        return self.columns.index(name)


def gamecheck(engine, family):
    engine.GameType = family
    engine.HasResource = lambda *args: False
    return support.load_script('GameCheck.py', {
        'GemRB': engine, 'GUIDefines': support.DEFINES,
        'ie_restype': support.load_script('ie_restype.py'),
    })


def native_window(engine, family):
    # Native BGEE and BG2EE GUIREC/3 share text110 and info/HLA125/126;
    # classic BG1 has text42. Return None for the other layout's text control.
    ids = set(range(1, 130)) | {0} | set(range(0x10000000, 0x10000100))
    ids.remove(42 if family != 'bg1' else 110)
    window = support.Window(engine, ids)
    window.deleted = []
    window.DeleteControl = lambda number: window.deleted.append(number)
    for control in window.controls.values():
        control.MakeEscape = lambda: None
    return window


class NativeLevelupTests(unittest.TestCase):
    def test_actual_spell_levelup_uses_bgee_original_and_bg2ee_new_ids(self):
        for family in ('bgee', 'bg2ee', 'bg2'):
            with self.subTest(family=family):
                engine = support.Engine({
                    'mxsplsrc': Table([str(i) for i in range(1,10)],
                        [(str(i), [3 if i == 1 else 6] + [0]*8) for i in range(7)]),
                    'splsrc': Table([str(i) for i in range(1,10)],
                        [(str(i), [2 if i == 1 else 4] + [0]*8) for i in range(7)]),
                })
                check = gamecheck(engine, family)
                modules = support.dependencies(engine, SimpleNamespace())
                modules['GameCheck'] = check
                modules['Spellbook'] = SimpleNamespace(
                    GetSpellLearningTable=lambda table: 'splsrc',
                    GetMageSpells=lambda *args: [['SPWI103', 0], ['SPWI104', 0]],
                    HasSpell=lambda *args: -1,
                    HasSorcererBook=lambda pc: 2,
                )
                module = support.load_script('LUSpellSelection.py', modules)
                ids = set(range(25 if family != 'bg2' else 24))
                ids |= {0x10000018}
                ids |= {40,41,42} if family == 'bg2ee' else {26,27,28}
                engine.window = support.Window(engine, ids)
                for control in engine.window.controls.values():
                    control.SetVisible = lambda value: None
                # Classic BG2 creates a scrollbar; EE resource layouts retain
                # their native controls. Track it without modeling rendering.
                original_scrollbar = engine.window.CreateScrollBar
                def scrollbar(*args):
                    control = original_scrollbar(*args)
                    control.SetVisible = lambda value: None
                    return control
                engine.window.CreateScrollBar = scrollbar
                shown = []
                module.ShowSpells = lambda: shown.append(module.SpellLevel)
                module.OpenSpellsWindow(1, 'mxsplsrc', 6, 5)
                done, text = (42,41) if family == 'bg2ee' else (28,26)
                self.assertIs(module.DoneButton, engine.window.GetControl(done))
                self.assertIs(module.SpellsTextArea, engine.window.GetControl(text))
                self.assertEqual(module.SpellsSelectPointsLeft[0], 2)
                self.assertEqual(shown, [0])
                self.assertIsNotNone(engine.window.modal)
                self.assertEqual(engine.window.created_buttons, [24] if family == 'bg2' else [])

    def test_actual_spell_learning_uses_known_growth_when_slots_are_unchanged(self):
        for family in ('bgee', 'bg2ee', 'bg2'):
            for spontaneous in (False, True):
                with self.subTest(family=family, spontaneous=spontaneous):
                    engine = support.Engine({
                        'MXSPLSRC': Table([str(i) for i in range(1,10)],
                            [('6', [6,5,3,0,0,0,0,0,0]), ('8', [6,6,5,3,0,0,0,0,0])]),
                        'SPLSRCKN': Table([str(i) for i in range(1,10)],
                            [('6', [4,2,1,0,0,0,0,0,0]), ('8', [5,3,2,1,0,0,0,0,0])]),
                    })
                    modules = support.dependencies(engine, SimpleNamespace())
                    modules['GameCheck'] = gamecheck(engine, family)
                    modules['Spellbook'] = SimpleNamespace(
                        GetSpellLearningTable=lambda table: 'SPLSRCKN',
                        GetMageSpells=lambda *args: [['SPWI103', 0]], HasSpell=lambda *args: -1,
                        HasSorcererBook=lambda pc: 2 if spontaneous else 0,
                    )
                    module = support.load_script('LUSpellSelection.py', modules)
                    engine.window = support.Window(engine)
                    original_scrollbar = engine.window.CreateScrollBar
                    def scrollbar(*args):
                        control = original_scrollbar(*args)
                        control.SetVisible = lambda value: None
                        return control
                    engine.window.CreateScrollBar = scrollbar
                    # This test isolates learning eligibility from the separately
                    # covered native control layouts and from spell rendering.
                    module.UpdateScrollBar = lambda *args: None
                    module.ShowSpells = lambda: None
                    module.OpenSpellsWindow(1, 'MXSPLSRC', 8, 2)
                    self.assertEqual(module.SpellsSelectPointsLeft[:4],
                                     [1,1,1,1] if spontaneous else [0,1,1,1])
                    self.assertEqual(module.SpellLevel, 0 if spontaneous else 1)

    def test_skill_plus_uses_present_textarea_and_allocates_component_points(self):
        for family in ('bgee', 'bg2ee', 'bg2', 'bg1'):
            with self.subTest(family=family):
                engine, module, _ = support.SkillSelectionTests().setup_selection()
                module.GameCheck = gamecheck(engine, family)
                window = native_window(engine, family)
                callbacks = []
                module.SetupSkillsWindow(1, module.LUSKILLS_TYPE_LEVELUP, window,
                                         lambda: callbacks.append('redraw'), [1, 1], [1, 2])
                text_id = 42 if family == 'bg1' else 110
                self.assertIs(module.SkillsTextArea, window.GetControl(text_id))
                self.assertEqual(engine.GetVar('SkillPointsLeft'), 10)
                module.SkillIncreasePress(SimpleNamespace(Value=0))
                self.assertEqual(engine.GetVar('SkillPointsLeft'), 9)
                self.assertEqual(engine.GetVar('Skill 0'), 1)
                self.assertEqual(callbacks, ['redraw'])
                self.assertEqual(window.GetControl(text_id).texts[-1][0], 12001)

    def test_ee_proficiency_selection_initializes_native_controls(self):
        for family in ('bgee', 'bg2ee', 'bg2'):
            with self.subTest(family=family):
                names = {22: 'SORCERER_MONK', 19: 'SORCERER', 20: 'MONK'}
                rows = [(f'LEGACY{i}', [i, 1100+i, 1200+i, 0]) for i in range(8)]
                rows += [(name, [90+i, 2100+i, 2200+i, 1]) for i, name in enumerate(
                    ('DAGGER', 'STAFF', 'SLING', 'DART', 'CLUB', 'SPEAR', '2WEAPON'))]
                engine = support.Engine({
                    'profs': Table(['FIRST_LEVEL', 'RATE'], [('SORCERER_MONK', [2, 3]),
                                   ('SORCERER', [1, 6]), ('MONK', [2, 3])]),
                    'weapprof': Table(['ID', 'NAME', 'DESC', 'SORCERER_MONK'], rows),
                })
                common = SimpleNamespace(
                    IsDualClassedDetailed=lambda pc: (0, 0, 0),
                    IsMultiClassed=lambda *args: (2, 19, 20, 0),
                    GetClassRowName=lambda value, *args: names[value], GetKitIndex=lambda pc: 0,
                )
                modules = support.dependencies(engine, common)
                modules['GameCheck'] = gamecheck(engine, family)
                module = support.load_script('LUProfsSelection.py', modules)
                window = native_window(engine, family)
                module.SetupProfsWindow(1, module.LUPROFS_TYPE_LEVELUP, window,
                                        lambda: None, [1, 2], [1, 3])
                self.assertIs(module.ProfsTextArea, window.GetControl(110))
                self.assertEqual(module.ProfsTableOffset, 8)
                self.assertEqual(module.ProfsNumButtons, 7)
                self.assertEqual(engine.GetVar('ProfsPointsLeft'), 1)
                module.ProfsLeftPress(SimpleNamespace(Value=0))
                self.assertEqual(engine.GetVar('ProfsPointsLeft'), 0)
                self.assertEqual(engine.GetVar('Prof 0'), 1)

    def test_actual_open_levelup_routes_native_layout_and_preserves_bg1(self):
        source = (support.SCRIPTS / 'LevelUp.py').read_text()
        functions = [node for node in ast.parse(source).body if isinstance(node, ast.FunctionDef)
                     and node.name in ('OpenLevelUpWindow', 'HideSkills', 'RedrawSkills')]
        for family in ('bgee', 'bg2ee', 'bg2', 'bg1'):
            with self.subTest(family=family):
                engine = support.Engine({})
                engine.window = native_window(engine, family)
                engine.GameGetSelectedPCSingle = lambda: 1
                engine.GetPlayerName = lambda pc: 'synthetic'
                check = gamecheck(engine, family)
                prof_calls = []
                skill_calls = []
                actor = SimpleNamespace(
                    classid=22, isdual=False, ClassTitle=lambda: 'SORCERER_MONK', KitIndex=lambda: 0,
                    ClassNames=lambda: ['SORCERER', 'MONK'], Classes=lambda: [19, 20],
                    NumClasses=lambda: 2, Levels=lambda: [1, 1], NextLevels=lambda: [1, 2],
                    LevelDiffs=lambda: [0, 1],
                )
                common = SimpleNamespace(
                    GetActorClassTitle=lambda pc: 'SORCERER_MONK', GetKitIndex=lambda pc: 0,
                    GetClassRowName=lambda value, *args: 'SORCERER_MONK',
                    GetKitRowName=lambda *args: 'SORCERER_MONK',
                    IsMultiClassed=lambda *args: (2, 19, 20, 0),
                )
                namespace = {name: getattr(support.STATS, name) for name in dir(support.STATS) if name.startswith('IE_')}
                namespace.update({name: getattr(support.DEFINES, name) for name in dir(support.DEFINES) if not name.startswith('_')})
                namespace.update({
                    'GemRB': engine, 'GameCheck': check, 'GUICommon': common,
                    'Actor': SimpleNamespace(Actor=lambda pc: actor),
                    'LUCommon': SimpleNamespace(GetNextLevels=lambda *args: [1, 2, 0],
                        GetLevelDiff=lambda *args: [0, 1, 0], SetupSavingThrows=lambda *args: None,
                        SetupThaco=lambda *args: None, SetupLore=lambda *args: None, SetupHP=lambda *args: None),
                    'LUProfsSelection': SimpleNamespace(LUPROFS_TYPE_LEVELUP=1,
                        SetupProfsWindow=lambda *args: prof_calls.append(args)),
                    'LUSkillsSelection': SimpleNamespace(LUSKILLS_TYPE_LEVELUP=1,
                        SetupSkillsWindow=lambda *args: skill_calls.append(args)),
                    'Spellbook': SimpleNamespace(HasSorcererBook=lambda *args: False),
                    'GetNewSpells': lambda *args: None, 'GetLevelUpNews': lambda: 'news',
                    'LevelUpInfoPress': lambda: None, 'LevelUpDonePress': lambda: None,
                    'OldSaves': [0]*5,
                })
                exec(compile(ast.Module(body=functions, type_ignores=[]), 'LevelUp callbacks', 'exec'), namespace)
                with patch.dict(sys.modules, {'GUIREC': SimpleNamespace(GetStatOverview=lambda *args: 'overview')}), redirect_stdout(io.StringIO()):
                    namespace['OpenLevelUpWindow']()
                text_id = 42 if family == 'bg1' else 110
                self.assertIs(namespace['TextAreaControl'], engine.window.GetControl(text_id))
                self.assertIsNotNone(engine.window.modal)
                self.assertEqual(len(prof_calls[0]), 9 if family == 'bg1' else 6)
                self.assertEqual(len(skill_calls[0]), 8 if family == 'bg1' else 6)
                self.assertEqual(0x1000007e in engine.window.deleted, family != 'bg1')


if __name__ == '__main__':
    unittest.main()
