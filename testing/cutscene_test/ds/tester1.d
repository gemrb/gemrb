BEGIN ~TESTER1~

IF ~NumTimesTalkedTo(0)~ THEN BEGIN RunTest
   SAY ~Cutscene test will begin now, please wait, it will length approximately 5 seconds"~
   IF ~~ THEN DO ~ActionOverride("tester2",MoveToPoint([1600.510]))
                 ActionOverride("tester2",MoveToPoint([1315.510]))
                 ActionOverride("tester2",MoveToPoint([1600.510]))
                 ActionOverride("tester2",MoveToPoint([1315.510]))
                 ActionOverride("tester2",MoveToPoint([1600.510]))
                 ActionOverride("tester2",MoveToPoint([1315.510]))
                 ActionOverride("tester2",MoveToPoint([1600.510]))
                 ActionOverride("tester2",MoveToPoint([1315.510]))
                 ActionOverride("tester2",MoveToPoint([1600.510]))
                 ActionOverride("tester2",MoveToPoint([1315.510]))
                 ActionOverride("tester2",MoveToPoint([1600.510]))
                 ActionOverride("tester2",MoveToPoint([1315.510]))
                 ActionOverride("tester2",MoveToPoint([1600.510]))
                 ActionOverride("tester2",MoveToPoint([1315.510]))
                 ActionOverride("tester2",MoveToPoint([1600.510]))
                 ActionOverride("tester2",MoveToPoint([1315.510]))
                 ActionOverride("tester2",MoveToPoint([1600.510]))
                 ActionOverride("tester2",MoveToPoint([1315.510]))
                 ActionOverride("tester2",MoveToPoint([1600.510]))
                 ActionOverride("tester2",DisplayString(Myself, @3))
                 Wait(5)
                 SetGlobal("TestCutsceneRunning", "GLOBAL", 1)
                 DisplayString(Myself, @1)
                 StartCutSceneMode()
                 Wait(5)
                 DisplayString(Myself, @2)
                 StartCutScene("cuttest")~ EXIT
END

IF ~NumTimesTalkedTo(1)~ THEN BEGIN TestRan
   SAY ~Test have been ran, here are the results~
   IF ~~ THEN DO ~SetGlobal("TestCutsceneRunning", "GLOBAL", 0)
                  SetNumTimesTalkedTo(2)
                  Dialogue(Player1)~ EXIT
END

IF ~Global("CutsceneIdDoesNotFreezeScript", "GLOBAL", 1)
   NumTimesTalkedTo(2)~ THEN BEGIN DoesNotFreezeScript
   SAY ~CutSceneId does not freeze script~
   IF ~~ THEN DO ~SetNumTimesTalkedTo(3)
                  Dialogue(Player1)~ EXIT
END

IF ~!Global("CutsceneIdDoesNotFreezeScript", "GLOBAL", 1)
   NumTimesTalkedTo(2)~ THEN BEGIN DoesFreezeScript
   SAY ~CutSceneId does freeze script~
   IF ~~ THEN DO ~SetNumTimesTalkedTo(3)
                  Dialogue(Player1)~ EXIT
END

IF ~Global("CutsceneDoesNotFreezeOtherActorsScript", "GLOBAL", 1)
   NumTimesTalkedTo(3)~ THEN BEGIN DoesNotFreezeOthersScript
   SAY ~CutScene does not freeze other actors scripts~
   IF ~~ THEN DO ~SetNumTimesTalkedTo(0)~ EXIT
END

//IE scripting really lacks ELSE!!
IF ~!Global("CutsceneDoesNotFreezeOtherActorsScript", "GLOBAL", 1)
   NumTimesTalkedTo(3)~ THEN BEGIN DoesFreezeOthersScript
   SAY ~CutScene does freeze other actors scripts~
   IF ~~ THEN DO ~SetNumTimesTalkedTo(0)~ EXIT
END

