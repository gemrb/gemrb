# Contributing to GemRB

First off, thanks for taking the time to contribute!

GemRB is a community driven Free/Libre Open Source Software project that welcomes all. It would not be possible
without the help of hundreds of volunteers like you, so everyone is encouraged
to contribute, be it code, knowledge, fun or something else!

This file is a non-exhaustive set of soft guidelines and ideas for contributing to GemRB.

  - [Where to start?](#where-to-start)
    + [Programmers](#programmers)
    + [Modders, game designers and artists](#modders-game-designers-and-artists)
    + [Everyone else](#everyone-else)
  - [But I am completely new to open source, git and GitHub!!](#but-i-am-completely-new-to-open-source-git-and-github)
  - [Axioms of Style](#axioms-of-style)
  - [Useful links](#useful-links)
  - [Useful tools](#useful-tools)
  - [Version tracking](#version-tracking)
  - [For maintainers](#for-maintainers)

Please stay friendly and constructive, we are not robots. I promise.

## Where to start?

### Programmers

If you don't trust your skills, take a look at the `good first issue` 
[subset of reports](https://github.com/gemrb/gemrb/labels/good%20first%20issue), ranging from trivial to simple.

Instructions on building and IDE setup can be found in INSTALL.

If you don't know what to work on:
- try playing a game with GemRB and make note of any bugs, annoyances or missing features ... then report and fix them
- check the list on the [bugtracker](https://github.com/gemrb/gemrb/issues),
- check out the FIXMEs and TODOs spread throughout the code,
- write one of the [missing file format plugins](https://github.com/gemrb/gemrb/issues/164) (doesn't require any game!)
- pick some of the [janitorial](https://github.com/gemrb/gemrb/labels/janitorial) tasks (eg. refactoring)
- ask either on the forums or chat.

You can also help by:
- reviewing [pull requests](https://github.com/gemrb/gemrb/pulls)
- reviewing [commits](https://github.com/gemrb/gemrb/commits/master) (subscribe to 
emails at [gemrb-commits](https://sourceforge.net/p/gemrb/mailman/))
- researching the behaviour of the originals:
  - reports tagged with [research needed](https://github.com/gemrb/gemrb/labels/research%20needed)
  - other open [questions](http://www.gemrb.org/wiki/doku.php?id=developers:ietesting)
  - reverse engineering the original binaries (there is an IDA pro db available for a faster start)
  - check questions about IESDP (forum)

If you're a web developer, see this [report](https://github.com/gemrb/gemrb/issues/116).


### Modders, game designers and artists
1. GemRB comes with a bundled demo, but it is trivial. The challenge is to enhance it, but
a lot of (compatibly licensed) art is missing. See also the docs on
[creating a new game](http://www.gemrb.org/wiki/doku.php?id=newgame:newgame)

2. Some GemRB mods are in the [gemrb-mods repo](https://github.com/lynxlynxlynx/gemrb-mods).
If you know WeiDU, a good task would be to merge them into
a tweakpack (10pp should stay alone though). And/or enhance them, they come with their own TODOs.

3. The [mod idea page](http://www.gemrb.org/wiki/doku.php?id=developers:mods) also lists 
several ideas from simple tweaks to more complex mods. 
Many could be added to the tweakpack under point 2, while others deserve independent projects.

4. Modders can also contribute with research (see above), especially when it requires WeiDU test beds.


### Everyone else

The easiest way for you to contribute is to take GemRB for a spin and **report problems, 
omissions, bugs and wishes** on the [tracker](https://github.com/gemrb/gemrb/issues/new/choose).

Some of the biggest progress has been made when power users tested features in great detail,
comparing the original with GemRB behaviour. The games and their engines are complex, so
programmers alone can't figure everything out. You can help out by sharing your knowledge in existing
reports on the tracker.

Take an active part in the GemRB and Gibberlings3 communities, helping other users, proposing
ideas and representing our "colours".

You can help spread the word about the project outside the main channels. Try to bring in new contributors,
blog, vlog (Let's Play?), social media posts, organize an event, write a song or other undefined creative approaches.

You can write, film or sing documentation, whether it is included with the project (eg. website, this file)
or not (forum, youtube ...).

Let us know of any articles, buzz or other materials you find about GemRB.

You can suggest cool screenshots or fanart for the gallery and videos
for the [video playlist](https://www.youtube.com/playlist?list=PL0AE43FB55973C06A).

If you're a web designer, see this [report](https://github.com/gemrb/gemrb/issues/116).

And finally, do what you want, as long as it is beneficial to the project. :)


## But I am completely new to open source, git and GitHub!!

Worry not and check these resources:
- A course on [github and pull requests](https://egghead.io/courses/how-to-contribute-to-an-open-source-project-on-github)
- [First contributions](https://github.com/multunus/first-contributions) is a hands-on tutorial that walks
you through contributions workflow on github. Scroll down to see the text.

Contributing to Free Software and open source projects can be a life changing experience. And it should
be fun, so if you find any of this hard to figure out, let us know
so we can improve our process and documentation!


## Axioms of Style

1. When in doubt, follow the style of the existing function or file.
   - When creating a new file, follow the style of existing files, pe. Game.h/.cpp.
     - Do not forget to include the license header.
2. Code indentation is done with single tabulators.
3. Spaces around operators (foo += bar;).
4. Try to avoid creating very long lines. There is no set maximum.
5. Sort includes by type (module, project, system) and alphabetically.


## Useful links
- [IESDP](https://gibberlings3.github.io/iesdp/), 
- [engine overview](http://www.gemrb.org/wiki/doku.php?id=engine:start), 
- [GUIScript documentation](http://www.gemrb.org/wiki/doku.php?id=documentation),
- [plaintext dumps](http://lynxlynx.info/ie/string-dumps.zip) of game strings with matching strrefs,
- the buildbots are accessible through any PR or commit
- the tools mentioned in the README links section


## Useful tools
- gdb
  You can use the following to automatically run gemrb and already start with a
  breakpoint in `abort()`:
    `gdb -q -iex "set breakpoint pending on" -iex "b abort" -ex run --args path/to/gemrb -c agame.cfg`
  (available as a script in `admin/run.gdb`)
- valgrind
  We bundle a suppression file for upstream problems at testing/python.supp
  You can use it for example this way (if running from the build dir):
    `valgrind --track-origins=yes --suppressions=../testing/python.supp gemrb/gemrb -c agame.cfg &> valgrind.log`
  (available as a script in `admin/run.valgrind`)
- `git diff` display for IE binary formats
  Check out `admin/enable-ie-git-diff` if you want `git log` and others to be able
  to display diffs of included binary files (spells and other overrides).


## Version tracking

1. Split your changes (commits) into well-rounded units of logic (`git commit -p` can help).
2. Each commit should compile and run. You can do quick fixups with `git commit --amend -p`.
3. Commit messages should be descriptive (why is more important than what, include area/creature/script names if possible).
4. Rebasing and force pushing to pull request branches is fine.

All of this makes reviewing and bisecting for regressions easier.

## For maintainers

1. Squash merge only if the history is a mess or it makes more sense (eg. consecutive commits through the github website). 
2. For release planning check the milestones and gemrb/docs/Release.txt.

