#!/bin/bash
# GemRB - Infinity Engine Emulator
# Copyright (C) 2009 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
# this script takes the guiscript docs and prepares them to shine as markdown

out_dir="${1:-$PWD/guiscript-docs.wikified}"
plugincpp="${2:-gemrb/plugins/GUIScript/GUIScript.cpp}"

mkdir -p "$out_dir"
rm -f "$out_dir"/*.md

function dumpDocs() {
  # extract the functions docs from the doc strings into individual files
  sed -n '/PyDoc_STRVAR/,/PyObject/ { /"=/,/"$/ { s,\\n,,; s,\\,,; p} }' "$plugincpp" |
    awk -v RS='"' -v out="$out_dir" \
    '{ split($0, t)
       title=t[2]
       if (title) {
         filename = out "/" title ".md"
         module = index(title, "_") ? "_GemRB" : "GemRB"
         # remove old title
         sub("^=====.*=====.", "")
         # fix See also links
         $0 = gensub("\\[\\[guiscript:(\\w+)\\]\\]", "[\\1](\\1.md)", "g")
         # also make some examples nicer until we convert the format
         sub("\*\*Examples*:..", "&\n")
         gsub("  [a-zA-Z0-9#]+", "  &")

         # dump frontmatter and contents
         print "---" > filename
         print "title: " title >> filename
         print "module: " module >> filename
         print "---" >> filename
         print $0 >> filename
       }
     }' || return 1

  echo "Done, now manually copy the contents of $out_dir to wiki/data/pages/guiscript"
  echo "Tar it up and move to FRS via the web UI or via SCP, eg.:"
  echo "  scp gsd.tgz USERNAME@frs.sourceforge.net:/home/frs/project/gemrb"
}

# old logic switch; all paths or no paths
dumpDocs
