How to make a release
*********************

Cleanup and pre-testing
=======================
Check if there are any open patches, pull requests or important stashes
laying around. Engage, triage and finalize/delay. It sucks to be waiting
for feedback too long, especially if you're an outsider.

Ideally make a speed run through one of the games to find any regressions.
BG2 intro area and TOB up to Saradush will test a lot of subsystems in a
short time.

Fix, revert or delay the release as needed.

Preparing and testing GIT
=========================

* Make sure that version numbers are correct for the upcoming version:
  * configure.in
  * gemrb/includes/globals.h

* Update NEWS with highlights since the last release 

* Make distribution .tar.gz (see Source release below) and test it
  - make sure all needed files are included
  - the minimal test should not fail:
    `gemrb/gemrb -c ../gemrb/GemRB.cfg.noinstall.sample`

* Tag current GIT to version number e.g. v0.9.0
  i.e. in the gemrb root directory do
```
    ver=0.8.2
    git tag -a -m "GemRB $ver" v$ver
    git push origin v$ver
```

* After you're done, update the version with a -git suffix, so it will be
  easier to tell if people are running release builds or not

Source release
==============

* via cmake/autotools (excludes useless files intentionally!):
  `make dist`

* via github (fallback):
  if you've pushed the new tagged release already, github will have generated
  a clean tarball for you at https://github.com/gemrb/gemrb/releases

Binary releases
===============
Check Travis and Appveyor build bots — the builds are automatically uploaded
to Sourceforge. Grab the commit hash and then mark the correct windows build
as the default download (see below).

If there's time/interest, manually generate an android build.

Release and Announcements
=========================

* Write the release notes if necessary. They are mainly for packagers, so
create them if there are structural or build related changes. New config
options should also be mentioned here if they're not part of the changelog.

* Put the tarballs/binaries into Releases on SF
  - go to the Files section
  - click on the sources subdir
  - add a new similarly-named folder
  - use "add file" to upload the sources and release notes
  - change the file properties to set the default platforms (under "view details")

* Test the downloads from sourceforge.net

* Announce on SF, #GemRB in channel and title, Discord channel, LGDB and Gibberlings3:
    - our forum
    - modding news (Lynx, Avenger, Mike1072, Theacefes, Grim Squeaker, DavidW and cmorgan have access)
  (a template is available in `admin/announcement.template`)
    - homepages: sidebar (versions and news), news, start (version), changelog (status
      and log swap), old_changelogs
    - github: go to the Releases tab and promote the new tag to a release, then add
      a link to the release notes

* Run `admin/restart_news.sh` to restart the NEWS cycle

* Run `admin/guidoc_wikifier.sh` (no params needed) and upload the new docs

