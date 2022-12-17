#!/bin/sh

d="$(dirname $0)"
cd "$d"

for game in bg1 bg2 iwd iwd2 pst how; do
    ./make_formation.py "$game" > ../gemrb/unhardcoded/"$game"/formatio.2da
done
