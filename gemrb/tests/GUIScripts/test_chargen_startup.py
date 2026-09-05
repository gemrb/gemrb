#!/usr/bin/env python3
"""Exercise the real chargen RunGame callback with deferred engine boundaries."""

import ast
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch


SOURCE = (Path(__file__).resolve().parents[2] / 'GUIScripts/bg2/CharGenEnd.py').read_text()


class StartupTests(unittest.TestCase):
    def run_start(self, family, book=0, imported=False, playmode=0, bgt=False):
        events = []
        timers = []
        actor = 7

        def record(name, *args):
            events.append((name, *args))

        def timed(callback, delay):
            self.assertIn(('enter',), events)
            self.assertEqual(delay, 1)
            timers.append(callback)

        gemrb = SimpleNamespace(
            GameSetExpansion=lambda value: record('expansion', value),
            GetVar=lambda name: {'PlayMode': playmode, 'ImportedChar': imported}[name],
            SaveCharacter=lambda pc, name: record('save-character', pc, name),
            EnterGame=lambda: record('enter'),
            SetTimedEvent=timed,
            ChargeSpells=lambda pc: record('charge', pc),
            ExecuteString=lambda command, pc: record('equip', command, pc),
            SetToken=lambda name, value: record('token', name, value),
            SetNextScript=lambda name: record('next', name),
        )
        gamecheck = SimpleNamespace(
            IsTOB=lambda: family == 'tob',
            HasTOB=lambda: family in ('tob', 'soa'),
            IsBGEE=lambda: family == 'bgee',
            IsBGT=lambda campaign: bgt and campaign == 'bg1',
        )
        tree = ast.parse(SOURCE)
        callback = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == 'RunGame')
        namespace = {
            'GemRB': gemrb, 'GameCheck': gamecheck,
            'Spellbook': SimpleNamespace(HasSorcererBook=lambda pc: book),
        }
        exec(compile(ast.Module(body=[callback], type_ignores=[]), 'CharGenEnd.RunGame', 'exec'), namespace)
        modules = {
            'CharGenCommon': SimpleNamespace(CharGenWindow=SimpleNamespace(Close=lambda: record('close'))),
            'CommonWindow': SimpleNamespace(SetGameGUIHidden=lambda hidden: record('hide', hidden)),
        }
        with patch.dict(sys.modules, modules):
            namespace['RunGame'](actor)
        # Both actions must really be deferred, not run synchronously during
        # chargen before EnterGame has established the actor's spellbook type.
        self.assertFalse(any(event[0] in ('charge', 'equip') for event in events))
        for callback in timers:
            callback()
        return events

    def test_fresh_bgee_spontaneous_books_charge_and_still_equip(self):
        # The engine book flag, not a hardcoded stock/custom class ID, governs
        # this path. Sorcerer and Sorcerer/Monk both expose arcane flag 2.
        for book in (2, 3):
            with self.subTest(book=book):
                events = self.run_start('bgee', book=book)
                self.assertEqual(events.count(('charge', 7)), 1)
                self.assertEqual(events.count(('equip', 'EquipMostDamagingMelee()', 7)), 1)
                self.assertNotIn(('hide', True), events)
                self.assertLess(events.index(('enter',)), events.index(('charge', 7)))

    def test_imported_bgee_actors_keep_their_saved_charges(self):
        for book in (0, 2, 3):
            with self.subTest(book=book):
                events = self.run_start('bgee', book=book, imported=True)
                self.assertNotIn(('charge', 7), events)
                self.assertIn(('equip', 'EquipMostDamagingMelee()', 7), events)

    def test_bgee_prepared_book_is_not_automatically_charged(self):
        events = self.run_start('bgee', book=0)
        self.assertNotIn(('charge', 7), events)
        self.assertIn(('equip', 'EquipMostDamagingMelee()', 7), events)

    def test_existing_tob_startup_path_is_unchanged(self):
        for imported in (False, True):
            with self.subTest(imported=imported):
                events = self.run_start('tob', book=2, imported=imported)
                self.assertIn(('expansion', 4), events)
                self.assertIn(('hide', True), events)
                self.assertEqual(events.count(('charge', 7)), 1)
                self.assertNotIn(('equip', 'EquipMostDamagingMelee()', 7), events)
        events = self.run_start('tob', book=0)
        self.assertNotIn(('charge', 7), events)
        self.assertIn(('equip', 'EquipMostDamagingMelee()', 7), events)

    def test_other_campaign_and_export_paths_are_unchanged(self):
        for family, bgt in (('soa', False), ('bg2', False), ('bg2', True)):
            with self.subTest(family=family, bgt=bgt):
                events = self.run_start(family, book=2, bgt=bgt)
                self.assertNotIn(('charge', 7), events)
                self.assertIn(('equip', 'EquipMostDamagingMelee()', 7), events)
                self.assertEqual(('hide', True) in events, not bgt)
        for family, next_script in (('bgee', 'Start'), ('tob', 'Start2')):
            with self.subTest(export=family):
                events = self.run_start(family, book=2, playmode=None)
                self.assertNotIn(('enter',), events)
                self.assertNotIn(('charge', 7), events)
                self.assertIn(('token', 'NextScript', next_script), events)
                self.assertIn(('next', 'ExportFile'), events)


if __name__ == '__main__':
    unittest.main()
