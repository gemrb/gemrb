#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile the real cast dispatch/check bodies with controlled engine boundaries.

This exercises Python reference ownership and exception conversion with CPython,
and actual actor/point/instant dispatch, without requiring proprietary game data.
It is not a replacement for the full engine build or live spell-casting tests.
"""

from pathlib import Path
import os
import shlex
import subprocess
import sysconfig
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


BOUNDARIES = r'''
#include <Python.h>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
using ieDword = unsigned int;
using ResRef = std::string;
using String = std::string;
template<class T> using Holder = std::shared_ptr<T>;
constexpr int GA_POINT=1, GA_NO_DEAD=2, GA_NO_HIDDEN=4, GA_NO_UNSCHEDULED=8;
constexpr int UI_SILENT=1, UI_NOAURA=2, UI_NOCHARGE=4;
enum class TargetMode { None, Cast };
namespace SpecialSpell { constexpr int AreaTarget=1; }
constexpr int TARGET_INVALID=0, TARGET_SELF=1, TARGET_NONE=2, TARGET_AREA=3,
              TARGET_CREA=4, TARGET_DEAD=5, TARGET_INV=6;
constexpr int MESSAGE=0, ERROR=1;
namespace HCStrings { constexpr int NoSeeNoCast=0; }
namespace GUIColors { constexpr int RED=0; }
namespace fmt { struct WideToChar { String value; }; }
template<class... T> void Log(T...) {}
struct Point { int x=0, y=0; };
struct Action {
    ResRef resref0Parameter;
    Point pointParameter;
    int int0Parameter=0, int1Parameter=0, int2Parameter=0;
};
struct CREMemorizedSpell { ResRef SpellResRef="actual"; };
struct SpellExtHeader {
    ResRef spellName="selected";
    int Target=TARGET_SELF, TargetNumber=1, type=0, level=0, slot=0, strref=0, Range=0;
};
struct Spellbook {
    CREMemorizedSpell memorized;
    SpellExtHeader info;
    bool present=true;
    int cleared=0;
    const CREMemorizedSpell* GetMemorizedSpell(int, int, int) const {
        return present ? &memorized : nullptr;
    }
    void ClearSpellInfo() { ++cleared; }
    void SetCustomSpellInfo(const std::vector<ResRef>& spells, const ResRef&, int) {
        if (!spells.empty()) { info.spellName=spells[0]; info.slot=-1; }
    }
    void GetSpellInfo(SpellExtHeader& out, int, int) const { out=info; }
    void FindSpellInfo(SpellExtHeader& out, const ResRef&, int) const { out=info; }
};
struct PCStatsType { ResRef QuickSpells[1]; int QuickSpellBookType[1]={1}; };
struct Actor {
    unsigned int InParty=1;
    Spellbook spellbook;
    PCStatsType* PCStats=nullptr;
    bool untargetable=false;
    std::vector<Holder<Action>> actions;
    unsigned int GetGlobalID() const { return 4242; }
    bool Untargetable(const ResRef&, const Actor*) const { return untargetable; }
    void Stop() {}
    void AddAction(Holder<Action> action) { actions.push_back(std::move(action)); }
};
Holder<Action> GenerateAction(std::string) { return std::make_shared<Action>(); }
Holder<Action> GenerateActionDirect(std::string, const Actor*) { return std::make_shared<Action>(); }
struct Display {
    void DisplayConstantStringName(int, int, Actor*) {}
} displayStorage;
Display* displaymsg=&displayStorage;
struct GameData { int GetSpecialSpell(const ResRef&) const { return 0; } } gameDataStorage;
GameData* gamedata=&gameDataStorage;
struct GameControl {
    std::function<bool(ieDword, const ResRef&)> spellCastCheck;
    ResRef spellName="prepared";
    int spellOrItem=0, spellSlot=0, spellIndex=0, spellCount=1, targetTypes=GA_POINT;
    TargetMode targetMode=TargetMode::Cast;
    void SetSpellCastCheck(std::function<bool(ieDword, const ResRef&)>);
    bool CheckSpellCast(const Actor*, const ResRef&) const;
    void TryToCast(Actor*, const Point&);
    void TryToCast(Actor*, const Actor*);
    void ResetTargetMode();
    void SetTargetMode(TargetMode mode) { targetMode=mode; } // omit cursor rendering only
    void DispatchGround(Actor* selectedActor, const Point& p);
    void SetupCasting(const ResRef& name, int type, int level, int index, int targets, int count) {
        spellName=name; spellOrItem=type; spellSlot=level; spellIndex=index;
        targetTypes=targets; spellCount=count; SetTargetMode(TargetMode::Cast);
    }
};
Actor caster;
GameControl control;
struct Dictionary { int Get(const char*, int fallback) const { return fallback; } };
struct Core {
    int applied=0;
    ResRef appliedSpell;
    Dictionary GetDictionary() const { return {}; }
    String GetString(int) const { return {}; }
    void ApplySpell(const ResRef& spell, Actor*, Actor*, int) { ++applied; appliedSpell=spell; }
} coreStorage;
Core* core=&coreStorage;
#define PARSE_ARGS(args, ...) if (!PyArg_ParseTuple(args, __VA_ARGS__)) return nullptr
#define GET_GAME()
#define GET_ACTOR_GLOBAL() Actor* actor=&caster
#define GET_GAMECONTROL() GameControl* gc=&control
PyObject* RuntimeError(const char* message) {
    PyErr_SetString(PyExc_RuntimeError, message);
    return nullptr;
}
'''


SCENARIOS = r'''
PyObject* globals;
void runPython(const char* code) {
    PyObject* result=PyRun_String(code, Py_file_input, globals, globals);
    if (!result) PyErr_Print();
    assert(result); Py_DECREF(result);
}
void install(const char* body) {
    runPython(body);
    PyObject* args=PyTuple_Pack(1, PyDict_GetItemString(globals, "check"));
    PyObject* result=GemRB_SetSpellCastCheck(nullptr, args);
    assert(result); Py_DECREF(result); Py_DECREF(args);
}
int calls() { return PyList_Size(PyDict_GetItemString(globals, "events")); }
void cast(int target, int type=1) {
    caster.spellbook.info.Target=target;
    PyObject* args=type==-3 ? Py_BuildValue("(iiis)",1,type,0,"variant") : Py_BuildValue("(iii)", 1, type, 0);
    PyObject* result=GemRB_SpellCast(nullptr, args);
    assert(result); Py_DECREF(result); Py_DECREF(args);
}
const char* allow="events=[]\ndef check(actor, spell):\n events.append((actor,spell)); return True\n";
const char* veto="events=[]\ndef check(actor, spell):\n events.append((actor,spell)); return False\n";
void checkEvents(const char* expected) {
    PyObject* result=PyRun_String(expected, Py_eval_input, globals, globals);
    assert(result && PyObject_IsTrue(result)); Py_DECREF(result);
}
int main(int argc, char** argv) {
    assert(argc==2);
    Py_Initialize();
    globals=PyModule_GetDict(PyImport_AddModule("__main__"));
    const std::string test=argv[1];
    if (test=="unchanged") {
        cast(TARGET_SELF); assert(caster.actions.size()==1);
        cast(TARGET_NONE); assert(core->applied==1);
        cast(TARGET_AREA); assert(caster.actions.size()==1);
        control.TryToCast(&caster, Point{}); assert(caster.actions.size()==2);
    } else if (test=="accepted") {
        install(allow);
        cast(TARGET_NONE); assert(core->applied==1 && calls()==1);
        assert(core->appliedSpell=="selected");
        cast(TARGET_SELF); assert(caster.actions.size()==1 && calls()==2);
        for (int target : {TARGET_CREA, TARGET_DEAD, TARGET_AREA}) {
            cast(target); int before=calls();
            if (target==TARGET_AREA) control.TryToCast(&caster, Point{});
            else { Actor victim; control.TryToCast(&caster, &victim); }
            assert(calls()==before+1);
        }
        assert(caster.actions.size()==4);
        checkEvents("events == [(1,'selected')] + [(1,'actual')]*4");
    } else if (test=="direct_substitution") {
        install(allow);
        caster.spellbook.present=false;
        cast(TARGET_SELF,-3); cast(TARGET_NONE,-3); cast(TARGET_AREA,-3);
        control.TryToCast(&caster,Point{});
        assert(caster.actions.size()==2 && core->applied==1);
        assert(caster.actions[0]->resref0Parameter=="variant");
        assert(caster.actions[1]->resref0Parameter=="variant");
        assert(core->appliedSpell=="variant");
        checkEvents("events == [(1,'variant')]*3");
    } else if (test=="veto") {
        install(veto);
        for (int target : {TARGET_NONE,TARGET_SELF,TARGET_CREA,TARGET_AREA}) {
            cast(target);
            if (target==TARGET_CREA) { Actor victim; control.TryToCast(&caster,&victim); }
            if (target==TARGET_AREA) control.TryToCast(&caster, Point{});
            assert(control.targetMode==TargetMode::None);
        }
        assert(calls()==4 && core->applied==0 && caster.actions.empty());
    } else if (test=="not_accepted") {
        install(allow);
        cast(TARGET_AREA); control.ResetTargetMode();
        assert(control.spellCount==1 && control.targetMode==TargetMode::None);
        control.DispatchGround(&caster,Point{}); // actual accepted-cast dispatch guard
        cast(TARGET_CREA); Actor victim; victim.untargetable=true;
        control.TryToCast(&caster,&victim);
        cast(TARGET_CREA); control.TryToCast(&caster,Point{}); // wrong target kind
        caster.spellbook.present=false;
        cast(TARGET_SELF); cast(TARGET_AREA); control.TryToCast(&caster,Point{});
        cast(TARGET_INV); cast(TARGET_INVALID); cast(TARGET_SELF,-1);
        assert(calls()==0 && core->applied==0 && caster.actions.empty());
        assert(caster.spellbook.cleared==1);
    } else if (test=="multitarget_veto") {
        install(veto);
        caster.spellbook.info.TargetNumber=2;
        cast(TARGET_AREA);
        control.DispatchGround(&caster,Point{});
        assert(calls()==1 && caster.actions.empty());
        // Native reset changes the cursor/mode, not the remaining target count.
        assert(control.spellCount==1 && control.targetMode==TargetMode::None);
        control.DispatchGround(&caster,Point{});
        assert(calls()==1 && caster.actions.empty());
    } else if (test=="items") {
        install(veto);
        for (bool point : {false,true}) {
            control.spellOrItem=-1; control.spellCount=1;
            control.targetTypes=GA_POINT; control.SetTargetMode(TargetMode::Cast);
            if (point) control.TryToCast(&caster,Point{});
            else control.TryToCast(&caster,&caster);
        }
        assert(calls()==0 && caster.actions.size()==2);
    } else if (test=="identity_and_targets") {
        install(allow);
        caster.InParty=0;
        control.spellIndex=-1; control.spellCount=2;
        control.TryToCast(&caster,Point{});
        assert(control.spellCount==1);
        control.TryToCast(&caster,Point{});
        assert(control.spellCount==0 && caster.actions.size()==2);
        checkEvents("events == [(4242,'prepared')]*2");
    } else if (test=="exceptions") {
        install("def check(*args): raise RuntimeError('callback veto')\n");
        cast(TARGET_NONE); cast(TARGET_SELF);
        assert(!PyErr_Occurred() && core->applied==0 && caster.actions.empty());
        install("class BadBool:\n def __bool__(self): raise RuntimeError('truth veto')\ndef check(*args): return BadBool()\n");
        cast(TARGET_AREA); control.TryToCast(&caster,Point{});
        assert(!PyErr_Occurred() && caster.actions.empty());
    } else if (test=="lifetime") {
        install(allow);
        PyObject* callback=PyDict_GetItemString(globals,"check");
        const Py_ssize_t installedRefs=Py_REFCNT(callback);
        control.SetSpellCastCheck(nullptr);
        assert(Py_REFCNT(callback)==installedRefs-1);
        {
            GameControl temporary;
            temporary.SetSpellCastCheck(PythonSpellCastCheck(callback));
            assert(Py_REFCNT(callback)==installedRefs);
        }
        assert(Py_REFCNT(callback)==installedRefs-1);
        static PyMethodDef clear={"clear_check", GemRB_SetSpellCastCheck, METH_VARARGS, nullptr};
        PyObject* method=PyCFunction_New(&clear,nullptr);
        PyDict_SetItemString(globals,"clear_check",method); Py_DECREF(method);
        install("def check(*args):\n clear_check(None)\n return True\n");
        cast(TARGET_SELF); cast(TARGET_NONE);
        assert(caster.actions.size()==1 && core->applied==1 && !control.spellCastCheck);
        PyObject* args=Py_BuildValue("(i)",7);
        assert(GemRB_SetSpellCastCheck(nullptr,args)==nullptr && PyErr_Occurred());
        Py_DECREF(args); PyErr_Clear();
    } else { assert(false); }
    control.SetSpellCastCheck(nullptr); // release Python state before finalization
    Py_Finalize();
    std::cout << test << " passed\n";
}
'''


class SpellCastCheckTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        game_control = (ROOT / "core/GUI/GameControl.cpp").read_text()
        gui_script = (ROOT / "plugins/GUIScript/GUIScript.cpp").read_text()
        callbacks = (ROOT / "plugins/GUIScript/PythonCallbacks.h").read_text()
        pieces = [BOUNDARIES]
        # Keep the production CPython ownership/error handling, not a fake callback.
        pieces.append(callbacks[callbacks.index("template<typename R>"):callbacks.index("template<class R, class ARG_T>")])
        for signature in (
            "void GameControl::SetSpellCastCheck(",
            "bool GameControl::CheckSpellCast(",
            "void GameControl::ResetTargetMode(",
            "void GameControl::TryToCast(Actor* source, const Point&",
            "void GameControl::TryToCast(Actor* source, const Actor*",
        ):
            pieces.append(function(game_control, signature))
        # Compile the exact ground-cast branch of the real input dispatcher.
        # Actor lookup and unrelated door/travel branches remain boundaries;
        # cancellation must be proved by this mode guard, not a fake count reset.
        dispatch = function(game_control, "void GameControl::PerformSelectedAction(")
        ground_cast = function(dispatch, "if (targetMode == TargetMode::Cast && !(gamedata->GetSpecialSpell")
        pieces.append("void GameControl::DispatchGround(Actor* selectedActor, const Point& p) {\n" + ground_cast + "\n}")
        pieces.append(function(gui_script, "struct PythonSpellCastCheck") + ";")
        pieces.append(function(gui_script, "static PyObject* GemRB_SetSpellCastCheck("))
        pieces.append(function(gui_script, "static PyObject* GemRB_SpellCast("))
        pieces.append(SCENARIOS)
        cls.temp = tempfile.TemporaryDirectory(prefix="gemrb-cast-check-")
        cls.addClassCleanup(cls.temp.cleanup)
        source = Path(cls.temp.name) / "cast.cpp"
        source.write_text("\n".join(pieces))
        cls.binary = source.with_suffix("")
        library = sysconfig.get_config_var("LDLIBRARY")
        libname = library.removeprefix("lib").split(".so")[0].split(".a")[0].split(".dylib")[0]
        command = shlex.split(os.environ.get("CXX", "c++")) + [
            "-std=c++17", "-O0", "-g", "-UNDEBUG",
            "-I" + sysconfig.get_path("include"), str(source),
            "-L" + sysconfig.get_config_var("LIBDIR"), "-l" + libname,
            *shlex.split(sysconfig.get_config_var("LIBS") or ""),
            *shlex.split(sysconfig.get_config_var("SYSLIBS") or ""),
            "-o", str(cls.binary),
        ]
        result = subprocess.run(command, capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)

    def scenario(self, name):
        result = subprocess.run([str(self.binary), name], capture_output=True, text=True, timeout=20)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn(name + " passed", result.stdout)

    def test_no_hook_preserves_native_dispatch(self): self.scenario("unchanged")
    def test_actor_point_self_none_after_validation(self): self.scenario("accepted")
    def test_direct_unknown_substitution_uses_effective_resref(self): self.scenario("direct_substitution")
    def test_false_vetoes_queue_and_instant_effect(self): self.scenario("veto")
    def test_multitarget_veto_resets_mode_without_falsely_clearing_count(self): self.scenario("multitarget_veto")
    def test_canceled_invalid_untargetable_and_unmemorized_do_not_commit(self): self.scenario("not_accepted")
    def test_items_do_not_invoke_spell_check(self): self.scenario("items")
    def test_actual_substitution_global_actor_id_and_multiple_targets(self): self.scenario("identity_and_targets")
    def test_python_and_truth_conversion_exceptions_fail_closed(self): self.scenario("exceptions")
    def test_unregister_gamecontrol_lifetime_and_invalid_registration(self): self.scenario("lifetime")


if __name__ == "__main__":
    unittest.main()
