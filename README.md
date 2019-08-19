# GemRB

[![Travis build status](https://travis-ci.org/gemrb/gemrb.svg?branch=master)](https://travis-ci.org/gemrb/gemrb)
[![AppVeyor build status](https://ci.appveyor.com/api/projects/status/k5atpwnihjjiv993?svg=true)](https://ci.appveyor.com/project/lynxlynxlynx/gemrb)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/17070b6b1608402b884123d8ecefa2a4)](https://www.codacy.com/app/gemrb/gemrb?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=gemrb/gemrb&amp;utm_campaign=Badge_Grade)
[![Coverity Badge](https://scan.coverity.com/projects/288/badge.svg)](https://scan.coverity.com/projects/gemrb)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/3101/badge)](https://bestpractices.coreinfrastructure.org/projects/3101)

## Introduction

[GemRB](http://gemrb.org) (Game Engine Made with preRendered Background) is a portable open-source
reimplementation of the Infinity Engine that underpinned Baldur's Gate,
Icewind Dale and Planescape: Torment. It sports a cleaner design, greater
extensibility and several innovations.
Would you like to create a game like Baldur's Gate?

To try it out you either need some of the ORIGINAL game's data or you can
get a tiny sneak peek by running the included trivial game demo.

The original game data has to be installed on a windows
partition and mounted or copied to your (Linux/Unix) filesystem, installed
with WINE or extracted manually from the CDs using the tool `unshield'.

What little documentation exists is mostly in gemrb/docs/en/ and
subdirectories, the gemrb.6 man page, this file and the website.

## Supported platforms

Supported systems (i.e. we got reports about successfully running GemRB):
* Linux x86, x86-64, ppc
* FreeBSD x86
* MS Windows
* various Macintosh systems (even pre x86) also should work ...
* some smart phones (Symbian, Android, other Linux-based, iOS)
* some consoles (OpenPandora, Dingoo)
* some exotic OSes (ReactOS, SyllableOS, Haiku, AmigaOS)

g++ 4.3 is known to miscompile gemrb.

## Requirements

See the INSTALL [file](https://github.com/gemrb/gemrb/blob/master/INSTALL).

## Contacts

There are several ways you can get in touch:
* [homepage](http://gemrb.org)
* [GemRB forum](http://gibberlings3.net/forums/index.php?showforum=91)
* [IRC channel](http://webchat.freenode.net/?channels=GemRB), #GemRB on the FreeNode IRC network
* [Discord channel](https://discord.gg/64rEVAk) (Gibberlings3 server)
* [Bug tracker](https://github.com/gemrb/gemrb/issues/new/choose)


## Useful links

Original engine research and data manipulation software:
* [IESDP](https://gibberlings3.github.io/iesdp/), documentation for the Infinity Engine file formats and more
* [Near Infinity](https://github.com/NearInfinityBrowser/NearInfinity/wiki), Java viewer and editor for data files
* [DLTCEP](http://forums.gibberlings3.net/index.php?showforum=137), MS Windows viewer and editor for data files
* [iesh](https://github.com/gemrb/iesh), IE python library and shell (for exploring data files)

Tools that can help with data installation:
* [WINE](http://www.winehq.org), Open Source implementation of the Windows API, useful for installing the games
* [Unshield](http://synce.sourceforge.net/synce/unshield.php), extractor for .CAB files created by InstallShield
