#!/bin/sh

# SPDX-FileCopyrightText: 2009 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

d="$(dirname $0)"
cd "$d"

for game in bg1 bg2 iwd iwd2 pst how; do
    ./make_formation.py "$game" > ../gemrb/unhardcoded/"$game"/formatio.2da
done
