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
  - [Development priorities](#development-priorities)
    + [Roadmap](#roadmap)

Please stay friendly and constructive, we are not robots. I promise.

## Where to start?

### Programmers

If you don't trust your skills, take a look at the `good first issue` 
[subset of reports](https://github.com/gemrb/gemrb/labels/good%20first%20issue), ranging from trivial to simple.

Instructions on building and IDE setup can be found in INSTALL or the
[online developer docs](https://gemrb.org/Dev-docs.html).

If you don't know what to work on:
- try playing a game with GemRB and make note of any bugs, annoyances
or missing features ... then report and fix them,
- check the list on the [bugtracker](https://github.com/gemrb/gemrb/issues),
- check the [Roadmap](#roadmap) to see what priorities we are currently working on,
- check out the FIXMEs and TODOs spread throughout the code,
- pick some of the [janitorial](https://github.com/gemrb/gemrb/labels/janitorial) tasks (eg. refactoring),
- work on tools like [iesh](https://github.com/gemrb/iesh/issues/18) (python), [Near Infinity](https://github.com/gemrb/gemrb/issues/2347) (java) or [content creation workflows](https://github.com/gemrb/gemrb/issues/1051),
- ask either on the forums or chat.

You can also help by:
- reviewing [pull requests](https://github.com/gemrb/gemrb/pulls)
- reviewing [commits](https://github.com/gemrb/gemrb/commits/master) (subscribe to 
emails at [gemrb-commits](https://sourceforge.net/p/gemrb/mailman/))
- researching the behaviour of the originals:
  - reports tagged with [research needed](https://github.com/gemrb/gemrb/labels/research%20needed)
  - reverse engineering the original binaries
  - check questions about IESDP ([forum](https://www.gibberlings3.net/forums/forum/54-iesdp-updates-and-info/))

If you're a web developer, check the website [reports](https://github.com/gemrb/gemrb.github.io/issues).


### Modders, game designers and artists

1. GemRB comes with a bundled demo, but it is short. The challenge is to enhance it, but
a lot of (compatibly licensed) art is missing. Check the current
[progress tracker](https://github.com/gemrb/gemrb/issues/1210). See also the docs on
[creating a new game](https://gemrb.org/New-game.html)

2. Some GemRB mods are in the [gemrb-mods repo](https://github.com/lynxlynxlynx/gemrb-mods).
The [mod idea page](https://gemrb.org/Modding.html) also lists
several ideas from simple tweaks to more complex mods. If you know WeiDU, a good task would be to add
more of the documented tweaks (idea page) to gemrb-tweakpack.

3. Modders can also contribute with research (see above), especially when it requires WeiDU test mods.


### Everyone else

The easiest way for you to contribute is to take GemRB for a spin and **report problems, 
omissions, bugs and wishes** on the [tracker](https://github.com/gemrb/gemrb/issues/new/choose).

Some of the biggest progress has been made when power users tested features in great detail,
comparing the original with GemRB behaviour. The games and their engines are complex, so
programmers alone can't figure everything out. You can help out by sharing your knowledge in existing
reports on the tracker.

Take an active part in the GemRB and Gibberlings3 communities, helping other users, proposing
ideas and representing our colours.

You can help spread the word about the project outside the main channels. Try to bring in new contributors,
blog, vlog (Let's Play?), social media posts, organize an event, write a song or other undefined creative approaches.

You can write, film or sing documentation, whether it is included with the project (eg. website, this file)
or not (forum, youtube ...).

Let us know of any articles, buzz or other materials you find about GemRB.

You can suggest cool screenshots or fanart for the gallery and videos
for the [video playlist](https://www.youtube.com/playlist?list=PL0AE43FB55973C06A).

If you're a web designer, check the website [repo](https://github.com/gemrb/gemrb.github.io)
and suggest improvements.

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
2. Clang-format is enforced as a pre-commit hook. It's best if you
configure your editor to run it for you. If you need to do it manually,
check out `git clang-format`.


## Useful links
- [IESDP](https://gibberlings3.github.io/iesdp/), 
- [engine overview](https://gemrb.org/Engine-overview.html),
- [GUIScript documentation](https://gemrb.org/GUIScript/Index.html),
- [plaintext dumps](http://lynxlynx.info/ie/string-dumps.zip) of game strings with matching strrefs,
- the buildbots are accessible through any PR, commit or badges in the README
- the tools mentioned in the README links section


## Useful tools
- gdb / `admin/run.gdb`

You can use the following to automatically run gemrb and already start with a
breakpoint in `abort()`:

    gdb -q -iex "set breakpoint pending on" -iex "b abort" -ex run --args path/to/gemrb -c agame.cfg

- valgrind / `admin/run.valgrind`

We bundle a suppression file for upstream problems at `testing/python.supp`.
You can use it for example this way (if running from the build dir):

    valgrind --track-origins=yes --suppressions=../testing/python.supp gemrb/gemrb -c agame.cfg &> valgrind.log

- `git diff` display for IE binary formats

Check out `admin/enable-ie-git-diff` if you want `git log` and others to be able
to display diffs of included binary files (spells and other overrides).

- `make test` will run the core test suite if you built with googletest installed

- `git blame` ignoring reformatting commits

The `.git-blame-ignore-revs` specifies which commits git should ignore when
finding the last time a line was changed. Many tools use it by default,
except git itself, so you need to run:

   git config --add blame.ignoreRevsFile .git-blame-ignore-revs

If you're using cmake, this has been done for you.


## Version tracking

1. Split your changes (commits) into well-rounded units of logic (`git commit -p` can help).
2. Read the commit diff to verify you're commiting what you think and no extraneous changes
   are included.
3. Each commit should compile and run. You can do quick fixups with `git commit --amend -p`.
4. Commit messages should be descriptive (why is more important than what, include
   area/creature/script names if possible).
5. Rebasing and force pushing to pull request branches is fine.

All of this makes reviewing and bisecting for regressions easier.


## For maintainers

1. Squash merge only if the history is a mess or it makes more
sense (eg. consecutive commits through the github website). 
2. For release planning check the milestones and `admin/github_release.checklist`
as a template to track progress in a dedicated issue.
3. Releases are usually made when larger pieces of work land or many smaller
fixes have accrued.
4. Versioning is semantic, but also ad-hoc. We want 1.0 to be the classic
polished release (feature complete or not), so we won't reach it until we
implement full iwd2 playability (you can finish the game). Until then, we'll
be bumping the patchlevel version number.


## Development priorities

The project has roughly two main priorities:
1. Fixing the known bugs and adding missing features

This is mainly what is on the bug tracker(s), most of the time not requiring
invasive changes.

2. Finishing larger missing pieces

This is represented by the "big picture" problems that remain to be solved.
It includes the [End of all rewrites](https://github.com/gemrb/gemrb/milestone/1)
milestone, the bigger PST bugs on the tracker, deciding what to do with the
incomplete OpenGL renderers and so on.

Generally each release tackles at least one item from #2 and a bag-of-holding
worth of #1.


### Roadmap

As noted in the previous section, most releases don't have very specific goals.
You can check what we're working towards in the current release by reading the
NEWS file and, as far as bugtracker backlog goes, by looking at the version's
[milestone](https://github.com/gemrb/gemrb/milestones).

The plan for 0.9.6 is to work on polishing, annoying pst oddities, maybe getting
bg2ee support out of experimental stage, deal with some postponed issues, and
whatever piques the contributor's fancy.

We're looking for ninjas to help with replacing FreeType (with SDL_ttf) and libvlc,
GLESv2 support (no hw, see [#938](https://github.com/gemrb/gemrb/issues/938)) and
a full-fledged installer for windows [#612](https://github.com/gemrb/gemrb/issues/612).
