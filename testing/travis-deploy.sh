#!/bin/bash
# script for sourceforge build deployment via travis
# relies on a separate ssh key for uploading
# uploads builds versioned by "git describe"

sshid=../id_travissfbot
if [[ $TRAVIS_OS_NAME == linux ]]; then
  filepath=Linux
  tarpath=iprefix
else
  echo "Unknown platform, exiting!"
  exit 13
fi

# there are no tags, so improvise
version=$({ date +%F; git describe --always; } | tr -d '\n') || exit 14
file=gemrb-$version.tar.bz2
tar cjf $file $tarpath || exit 15

filepath="$filepath/$file"
scp -i $sshid $file \
 gemrb-travisbot@frs.sourceforge.net:/home/frs/project/gemrb/Buildbot\\\ Binaries/$filepath
