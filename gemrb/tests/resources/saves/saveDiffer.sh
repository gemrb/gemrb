#!/bin/bash
# SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

gameType="$1"
baseGame="${2%????}" # strip extension
export PAGER=cat
pwd

if [[ -z $IEDIFF ]]; then
  echo "IEDIFF is not set to a path to iesh's iediff program! Skipping comparison!"
  exit 0
fi

# compare save contents
base1="../../../gemrb/tests/resources/saves/0 - $gameType"
base2="tests/resources/saves/000000001-Quick-Save"

files=( "$baseGame.sav" "$baseGame.gam" "worldmap.wmp" )
rc=10
for file in "${files[@]}"; do
  if [[ $file == "worldmap.wmp" && $gameType != ${gameType/ee/} ]]; then
    # ees moved the WMP into the SAV
    continue
  fi

  # deal with potential case issues
  a=$(find "$base1" -type f -iname "$file")
  b=$(find "$base2" -type f -iname "$file")
  echo "Diffing $file"
  echo "Running $IEDIFF --locase '$a' '$b'"
  diff=$("$IEDIFF" --locase "$a" "$b")
  if grep -q "are identical" <<< "$diff"; then
    echo "Files are equal!"
  else
    let rc++
    echo "$diff"
  fi
  echo
done

(( rc == 10 )) && exit 0 || exit $rc
