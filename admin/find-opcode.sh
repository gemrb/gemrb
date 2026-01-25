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

# a bit flaky opcode replacer
# takes two dwords, which need to be aligned to the 4-byte boundary or xxd will split them
# use the log and hexdump if necessary to understand the structure and get the starting value
# example run:
# replaceDW gemrb/unhardcoded/iwd2/turn.spl 0189 0025
function replaceDW() {
	f=$1
	indw=$(sed 's,\(..\)\(..\),\2\1,' <<< "$2")
	outdw=$(sed 's,\(..\)\(..\),\2\1,' <<< "$3")
	xxd "$f" | sed "s/$indw/$outdw/" | xxd -r > tmtmtmt3gfvxdfgfd
	mv tmtmtmt3gfvxdfgfd "$f"
	git diff "$f"
}
