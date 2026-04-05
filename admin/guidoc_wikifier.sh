#!/bin/bash

# SPDX-FileCopyrightText: 2009 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

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

         # dump frontmatter and contents
         print "---" > filename
         print "title: " title >> filename
         print "module: " module >> filename
         print "---" >> filename
         print $0 >> filename
       }
     }' || return 1

  echo "Done, now manually copy the contents of $out_dir to the website repo, review, commit and push."
}

dumpDocs
