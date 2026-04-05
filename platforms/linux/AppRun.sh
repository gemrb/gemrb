#!/bin/bash

# SPDX-FileCopyrightText: 2021 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

SCRIPT=$(readlink -f -- "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
APPDIR="${APPDIR:-$SCRIPTPATH}"

export PYTHONPATH="$APPDIR/usr/share/pyshared/"
export PYTHONHOME="$APPDIR/usr/"

"$APPDIR"/usr/bin/gemrb "$@"
