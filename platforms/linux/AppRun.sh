#!/bin/bash

SCRIPT=$(readlink -f -- "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
APPDIR="${APPDIR:-$SCRIPTPATH}"

export PYTHONPATH="$APPDIR/usr/share/pyshared/"
#echo ":$GITHUB_ACTIONS:"
#if [[ -z $GITHUB_ACTIONS ]]; then
  export PYTHONHOME="$APPDIR/usr/"
#fi

"$APPDIR"/usr/bin/gemrb "$@"
