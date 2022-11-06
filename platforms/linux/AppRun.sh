#!/bin/bash

SCRIPT=$(readlink -f -- "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
APPDIR="${APPDIR:-$SCRIPTPATH}"

export PYTHONPATH="$APPDIR/usr/share/pyshared/"
#export PYTHONHOME="$APPDIR/usr/"

"$APPDIR"/usr/bin/gemrb "$@"
