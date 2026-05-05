#!/bin/bash
# SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

gameType="$1"
baseGame="${2%????}" # strip extension
export PAGER=cat

if [[ -z $IEDIFF ]]; then
  echo "IEDIFF is not set to a path to iesh's iediff program! Skipping comparison!"
  exit 0
fi

# compare save contents
base1=$(realpath "../../../gemrb/tests/resources/saves/0 - $gameType")
base2=$(realpath "tests/resources/saves/000000001-Quick-Save")

function diffFile() {
  file="$1"
  # deal with potential case issues
  a=$(find "$base1" -type f -iname "$file")
  b=$(find "$base2" -type f -iname "$file")

  if md5sum "$a" "$b" | sort -u -k1 | wc -l | grep -q 1; then
    return 0
  fi

  diff=$("$IEDIFF" "$a" "$b")
  if ! grep -q "are identical" <<< "$diff"; then
    let rc++
    echo "Diffing $file"
    echo "Running $IEDIFF '$a' '$b'"
    echo "$diff"
  fi
}

files=( "$baseGame.sav" "$baseGame.gam" "worldmap.wmp" )
rc=10
for file in "${files[@]}"; do
  if [[ $file == "worldmap.wmp" && $gameType != ${gameType/ee/} ]]; then
    # ees moved the WMP into the SAV
    continue
  fi

  if [[ $file == "$baseGame.sav" ]]; then
    echo "Unpacking and inspecting $baseGame.sav, this can take a while."
    # unpack SAV then iterate the contents
    mkdir -p "$base1/sav" "$base2/sav"
    cd "$base1/sav"
    "${IEDIFF%/*}"/iezip -q -x "$base1/$baseGame.sav"
    cd -
    cd "$base2/sav"
    "${IEDIFF%/*}"/iezip -q -x "$base2/$baseGame.sav"
    cd -
    for subPath in "$base1/sav"/*; do
      subFile="${subPath##*/}"
      diffFile "$subFile"
    done
    rm -r "$base1/sav" "$base2/sav"
  else
    diffFile "$file"
  fi

  echo
done

(( rc == 10 )) && exit 0 || exit $rc
