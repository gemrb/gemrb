# GemRB

[![Travis build status](https://travis-ci.org/gemrb/gemrb.svg?branch=master)](https://travis-ci.org/gemrb/gemrb)
[![AppVeyor build status](https://ci.appveyor.com/api/projects/status/k5atpwnihjjiv993?svg=true)](https://ci.appveyor.com/project/lynxlynxlynx/gemrb)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/17070b6b1608402b884123d8ecefa2a4)](https://www.codacy.com/app/gemrb/gemrb?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=gemrb/gemrb&amp;utm_campaign=Badge_Grade)
[![Coverity Badge](https://scan.coverity.com/projects/288/badge.svg)](https://scan.coverity.com/projects/gemrb)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=gemrb_gemrb&metric=alert_status)](https://sonarcloud.io/dashboard?id=gemrb_gemrb)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/3101/badge)](https://bestpractices.coreinfrastructure.org/projects/3101)

## Introduction

[GemRB](http://gemrb.org) (Game Engine Made with preRendered Background) is a portable open-source
reimplementation of the Infinity Engine that underpinned Baldur's Gate,
Icewind Dale and Planescape: Torment. It sports a cleaner design, greater
extensibility and several innovations.
Would you like to create a game like Baldur's Gate?

To try it out you either need some of the ORIGINAL game's data or you can
get a tiny sneak peek by running the included trivial game demo.

The original game data has to be installed if you want to see anything but
the included trivial demo. On non-windows systems either copy it over from
a windows install, use a compatible installer, WINE or extract it manually
from the CDs using the unshield tool.

Documentation can be found on the [website](https://gemrb.github.io/Documentation),
while the sources are in gemrb/docs/en/ and the 
[gemrb.6 man page](http://www.gemrb.org/wiki/doku.php?id=docs:manpage).

If you want to help out, start by reading this
list of [options, tips and priorities](https://github.com/gemrb/gemrb/blob/master/CONTRIBUTING.md).

## Supported platforms

Architectures and platforms that successfully run or ran GemRB:
* Linux x86, x86-64, ppc, mips (s390x builds, but no running info)
* FreeBSD, OpenBSD, NetBSD
* MS Windows
* various Macintosh systems (even pre x86)
* some smart phones (Symbian, Android, other Linux-based, iOS)
* some consoles (OpenPandora, Dingoo)
* some exotic OSes (ReactOS, SyllableOS, Haiku, AmigaOS)

If you have something to add to the list or if an entry doesn't work any more, do let us know!

## Requirements

See the INSTALL [file](https://github.com/gemrb/gemrb/blob/master/INSTALL).

## Contacts

There are several ways you can get in touch:
* [Homepage](http://gemrb.org)
* [GemRB forum](https://www.gibberlings3.net/forums/forum/91-gemrb/)
* [IRC channel](http://webchat.freenode.net/?channels=GemRB), #GemRB on the FreeNode IRC network
* [Discord channel](https://discord.gg/64rEVAk) (Gibberlings3 server)
* [Bug tracker](https://github.com/gemrb/gemrb/issues/new/choose)


## Useful links

Original engine research and data manipulation software:
* [IESDP](https://gibberlings3.github.io/iesdp/), documentation for the Infinity Engine file formats and more
* [Near Infinity](https://github.com/NearInfinityBrowser/NearInfinity/wiki), Java viewer and editor for data files
* [DLTCEP](https://www.gibberlings3.net/forums/forum/137-dltcep/), MS Windows viewer and editor for data files
* [iesh](https://github.com/gemrb/iesh), IE python library and shell (for exploring data files)

Tools that can help with data installation:
* [WINE](http://www.winehq.org), Open Source implementation of the Windows API, useful for installing the games
* [Unshield](http://synce.sourceforge.net/synce/unshield.php), extractor for .CAB files created by InstallShield
