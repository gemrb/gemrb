#!/usr/bin/python
# GemRB - Infinity Engine Emulator
# Copyright (C) 2011 The GemRB Project
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
import sys

def usage(msg):
  print("Error:", msg)
  print("Usage:", sys.argv[0], 'filename APPEND|APPEND_COL "$|value1  [$|value2] ... [$|valueN]"')
  print("Passing $ will result in an empty cell. Use it for the first two entries when appending columns, so you don't break the 2da signature or default value")
  print()
  print("Example:")
  print("python extend2da.py gemrb/override/bg1/classes.2da APPEND 'HACKER 1 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0'")

# get the longest row for comparison, while caching the reads
def readAndGetMaxLength(f):
  global lines

  max = i = 0
  for line in f:
    lines.append(line.rstrip())
    if len(lines[i]) > max:
      max = len(lines[i])
    i = i + 1
  return max

def appendCol(f, max):
  global lines, data

  f.seek(0)
  f.truncate()
  i = 0
  for cell in data:
    if cell == "$":
      f.write(lines[i] + b"\n")
      i = i + 1
      continue
    padding = max - len(lines[i]) + 2 # 2 spaces as field separator
    f.write(lines[i] + (" "*padding + cell + "\n").encode('ascii'))

    i = i + 1

def CheckCountsMismatch(mode):
  global data, lines

  if mode == "APPEND_COL":
    msg = "rows"
    rc = 13
    count = len(lines)
  elif mode == "APPEND":
    msg = "columns"
    rc = 14
    count = len(lines[3].split()) # skip the 2da header
  else:
    return

  mismatch = count - len(data)
  if mismatch:
    print ("Error: you need to specify the data for all the %s!" % msg)
    print ("Table has %d, while you provided %d!" % (count, len(data)))
    sys.exit(rc)


def appendRow(f):
  global data

  # get a line with real content for measurement
  line = lines[3]
  vals = line.split()

  f.seek(0, 2) # seek to the end
  i = 0
  for cell in data:
    # width of cell (content + padding) from first line
    cw = len(line)-len(line.lstrip(vals[i]).lstrip())
    line = line[cw:]
    padding = cw - len(cell)
    if padding < 1:
      # ideally we would reformat the whole table, but meh
      padding = 2
    padding = " " * padding
    f.write((("%s"+padding) % cell).encode('ascii'))
    i += 1
  # remove the extraneus ending padding
  f.seek(f.tell()-2)
  f.truncate()
  f.write(b"\n")

################# MAIN ####################################
if len(sys.argv) < 4:
  usage("missing parameters")
  sys.exit(11)

filename = sys.argv[1]
mode = sys.argv[2].upper() # APPEND / APPEND_COL
data = sys.argv[3:][0].split()

if mode != "APPEND" and mode != "APPEND_COL":
  usage("invalid mode parameter")
  sys.exit(12)

lines = []

with open(filename, 'r+b') as f:
  max = readAndGetMaxLength(f)
  CheckCountsMismatch(mode)
  if mode == "APPEND_COL":
    appendCol(f, max)
  elif mode == "APPEND":
    appendRow(f)

# tests
# python2 extend2da.py gemrb/override/bg1/classes.2da APPEND_COL "$ $ BOGOSITY 0 0 0 00 0 0 0 0 0 0 0 0 0 0 2 3 4 7"
# python2 extend2da.py gemrb/override/bg1/classes.2da APPEND "HACKER 1 3 0 0 0 00 0 0 0 0 0 0 0 0 0 0 "

