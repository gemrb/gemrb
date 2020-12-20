//NOTE: weidu --game demo/ demo/data/source/riddler.d; mv -v riddler.dlg demo/data

BEGIN ~RIDDLER~ 7

IF ~GlobalLT("Riddles","GLOBAL",7)~ THEN BEGIN 0
  SAY #9 /* ~Greetings, <CHARNAME>. We have been waiting for you.~ */
  IF ~NumTimesTalkedTo(1)~ THEN REPLY #10 /* ~How do you know my name?~ */ GOTO 1
  IF ~!Global("SeenBlockade","GLOBAL",1)~ THEN REPLY #11 /* ~Who are you?~ */ GOTO 2
  IF ~!Global("SeenBlockade","GLOBAL",1)~ THEN REPLY #12 /* ~What is your purpose here?~ */ GOTO 2
  IF ~Global("SeenBlockade","GLOBAL",1) Global("Riddles","GLOBAL",0)~ THEN REPLY #13 /* ~Who are you, really?~ */ GOTO 4
  IF ~Global("SeenBlockade","GLOBAL",1) Global("Riddles","GLOBAL",0)~ THEN REPLY #14 /* ~I know more than before. What now?~ */ GOTO 4
  IF ~GlobalGT("Riddles","GLOBAL",0) GlobalLT("Riddles","GLOBAL",7)~ THEN REPLY #14 /* ~I know more than before.~ */ GOTO 5
  IF ~~ THEN REPLY #16 /* ~Err, ok. I'll be right back.~ */ EXIT
END

IF ~~ THEN BEGIN 1
  SAY #17 /* ~We know many things about you. You are like an open book.~ */
  IF ~~ THEN REPLY #18 /* ~...~ */ GOTO 0
END

IF ~~ THEN BEGIN 2
  SAY #19 /* ~I am but a pebble on your way to greater things. Return once you internalize this.~ */
  IF ~~ THEN REPLY #20 /* ~What do you mean?~ */ GOTO 3
  IF ~~ THEN REPLY #21 /* ~Hmpf. Very well.~ */ EXIT
END

IF ~~ THEN BEGIN 3
  SAY #22 /* ~Learn. Explore. Wonder. Delve.~ */
  IF ~~ THEN REPLY #23 /* ~Fine, whatever, see you later.~ */ EXIT
END

IF ~~ THEN BEGIN 4
  SAY #24 /* ~I am still but a pebble on your way to greater things. To proceed, you must prove your mental acuity.~ */
  IF ~~ THEN REPLY #25 /* ~My what?~ */ DO ~SetGlobal("Riddles", "GLOBAL", 1)~ GOTO 5
  IF ~~ THEN REPLY #26 /* ~Hmpf. Very well.~ */ EXIT
END

IF ~~ THEN BEGIN 5
  SAY #27 /* ~Solve a series of riddles to continue.~ */
  IF ~GlobalLT("Riddles","GLOBAL",2)~ THEN REPLY #28 /* ~Very well.~ */ GOTO 6
  IF ~Global("Riddles","GLOBAL",2)~ THEN REPLY #29 /* ~Very well, on to the second one.~ */ GOTO 7
  IF ~Global("Riddles","GLOBAL",3)~ THEN REPLY #30 /* ~Very well, on to the third one.~ */ GOTO 8
  IF ~Global("Riddles","GLOBAL",4)~ THEN REPLY #31 /* ~Very well, on to the fourth one.~ */ GOTO 9
  IF ~Global("Riddles","GLOBAL",5)~ THEN REPLY #32 /* ~Very well, on to the fifth one.~ */ GOTO 10
  IF ~Global("Riddles","GLOBAL",6)~ THEN REPLY #33 /* ~Very well, on to the next one.~ */ GOTO 11
  IF ~~ THEN REPLY #34 /* ~I'll be back later.~ */ EXIT
END

IF ~~ THEN BEGIN 6
  SAY #35 /* ~Who is old and always young, too long for some, too short for others?~ */
  IF ~~ THEN REPLY #36 /* ~Sight.~ */ GOTO 12
  IF ~~ THEN REPLY #37 /* ~Memory~ */ GOTO 12
  IF ~~ THEN REPLY #38 /* ~Life.~ */ GOTO 12
  IF ~~ THEN REPLY #39 /* ~Time.~ */ DO ~SetGlobal("Riddles","GLOBAL",2)~ GOTO 13
  IF ~~ THEN REPLY #40 /* ~Eternity.~ */ GOTO 12
END

IF ~~ THEN BEGIN 7
  SAY #41 /* ~Made of holes I hold it true. If you take one out from the middle, I split in two.~ */
  IF ~~ THEN REPLY #42 /* ~Net.~ */ GOTO 12
  IF ~~ THEN REPLY #43 /* ~Sieve.~ */ GOTO 12
  IF ~~ THEN REPLY #44 /* ~Fabric.~ */ GOTO 12
  IF ~~ THEN REPLY #45 /* ~Chain.~ */ DO ~SetGlobal("Riddles","GLOBAL",3)~ GOTO 13
END

IF ~~ THEN BEGIN 8
  SAY #46 /* ~It can be heard, but does not speak. Shows, but does not see. Moves, but never comes.~ */
  IF ~~ THEN REPLY #47 /* ~A message.~ */ GOTO 12
  IF ~~ THEN REPLY #48 /* ~A clock.~ */ DO ~SetGlobal("Riddles","GLOBAL",4)~ GOTO 13
  IF ~~ THEN REPLY #49 /* ~That deaf, dumb and blind kid.~ */ GOTO 12
  IF ~~ THEN REPLY #50 /* ~The sea.~ */ GOTO 12
END

IF ~~ THEN BEGIN 9
  SAY #51 /* ~I know and old grey guy that reaches sky high.~ */
  IF ~~ THEN REPLY #52 /* ~A mountain.~ */ GOTO 12
  IF ~~ THEN REPLY #53 /* ~A baloon.~ */ GOTO 12
  IF ~~ THEN REPLY #54 /* ~A thrown rock.~ */ GOTO 12
  IF ~~ THEN REPLY #55 /* ~Smoke.~ */ DO ~SetGlobal("Riddles","GLOBAL",5)~ GOTO 13
END

IF ~~ THEN BEGIN 10
  SAY #56 /* ~I come uninvited and can't be forced to come. If you want to see me, lay down and close your eyes.~ */
  IF ~~ THEN REPLY #57 /* ~Night.~ */ GOTO 12
  IF ~~ THEN REPLY #58 /* ~Sleep.~ */ DO ~SetGlobal("Riddles","GLOBAL",6)~ GOTO 13
  IF ~~ THEN REPLY #59 /* ~Darkness.~ */ GOTO 12
  IF ~~ THEN REPLY #60 /* ~A dream.~ */ GOTO 12
END

IF ~~ THEN BEGIN 11
  SAY #61 /* ~Sitting on the roof, looking in the kitchen.~ */
  IF ~~ THEN REPLY #62 /* ~A swallow.~ */ GOTO 12
  IF ~~ THEN REPLY #63 /* ~A dormouse.~ */ GOTO 12
  IF ~~ THEN REPLY #64 /* ~A chimney.~ */ DO ~SetGlobal("Riddles","GLOBAL",7)~ GOTO 14
  IF ~~ THEN REPLY #65 /* ~A burglar.~ */ GOTO 12
END

IF ~~ THEN BEGIN 12
  SAY #68 /* ~Incorrect. Return when you are ready.~ */
  IF ~~ THEN EXIT
END

IF ~~ THEN BEGIN 13
  SAY #69 /* ~That is correct.~ */
  IF ~~ THEN GOTO 5
END

IF ~~ THEN BEGIN 14
  SAY #70 /* ~That is correct and thus all the riddles are solved. You may pass.~ */
  IF ~~ THEN REPLY #71 /* Farewell, quizzical figure. */ DO ~SetGlobal("Riddles","GLOBAL",8) StartCutSceneMode() StartCutScene("scene01")~ EXIT
END

IF ~GlobalGT("Riddles","GLOBAL",6)~ THEN BEGIN 15
  SAY #15 /* ~Well done, I am of no further use to you.~ */
  IF ~~ THEN REPLY #71 /* Farewell, quizzical figure. */ EXIT
END
