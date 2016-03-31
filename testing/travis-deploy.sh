#!/bin/bash
# script for sourceforge build deployment via travis
# relies on a separate ssh key for uploading
# uploads builds versioned by "git describe"

sshid=../id_travissfbot
if [[ $TRAVIS_OS_NAME == osx ]]; then
  filepath=Apple/OSX
  tarpath=/Applications/GemRB.app
  # this can be removed in 2016 (the bug was fixed and just needs to be deployed)
  ssh-keyscan -t rsa,dsa -H frs.sourceforge.net 2>&1 | tee -a $HOME/.ssh/known_hosts
elif [[ $TRAVIS_OS_NAME == linux ]]; then
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

# No need to make anything a default download
# candidates: "default=windows&default=mac&default=linux&default=bsd&default=solaris&default=others"
#curl -H "Accept: application/json" -X PUT -d "default=linux" \
# -d "api_key=bot sf api key as per profile" \
# https://sourceforge.net/projects/gemrb/files/Buildbot\ Binaries/$filepath

