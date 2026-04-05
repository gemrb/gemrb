#!/bin/bash

# SPDX-FileCopyrightText: 2009 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# prepares NEWS for the restart of tracking bigger changes after release.

if [[ ! -e NEWS ]]; then
  echo 'Run me from the top gemrb dir that contains NEWS!'
  exit 3
fi

# get the last revision that contains a change in the word git in NEWS
# that's usually the final release update
rev=$(git log -Sgit --pretty="format:%h" NEWS | head -n 1)
rev="${rev:-missing revision}"

cat - NEWS > NEWSNEWS << LILARCOR
GemRB git ($rev):
  New features:
    - 

  Improved features:
    - 
    - bugfixes

LILARCOR
mv NEWSNEWS NEWS
