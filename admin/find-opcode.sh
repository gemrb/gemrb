#!/bin/bash
# checks if any of our binary files uses a particular effect opcode

IELISTER="ielister/ielister"

opcode="$1"
[[ -z $opcode ]] && echo "Pass an opcode number (dec or hex)" && exit 13

if [[ ${opcode:0:1} != "0" ]]; then
  # convert to hex
  printf -v opcode "%04x" $opcode
fi

find gemrb/unhardcoded gemrb/override demo/override \
  -regextype egrep -iregex ".*\.(spl|eff)$" |
while read file; do
  "$IELISTER" "$file" | grep -iq "Opcode   *$opcode" && echo "$file"
done | sort
