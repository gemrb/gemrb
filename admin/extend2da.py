#!/usr/bin/env python

# SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later
import sys

def usage(msg):
  print("Error:", msg)
  print("\nUsage:", sys.argv[0], 'filename APPEND|APPEND_COL "$|value1  [$|value2] ... [$|valueN]" [-r]\n"')
  print("\nUsage:", sys.argv[0], 'filename SWITCH "colName1 colName2"')
  print("Passing $ will result in an empty cell. Use it for the first two entries when appending columns,\nso you don't break the 2da signature or default value. Passing -r at the end will right-align cells.")
  print()
  print("Example:")
  print("python extend2da.py gemrb/override/bg1/classes.2da APPEND 'HACKER 1 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0'")

def isComment(line):
	if len(line) == 0:
		return False
	if len(line) > 0 and chr(line[0]) == "#":
		return True
	if len(line) > 1 and str(line[0:2]) == "//":
		return True
	return False

# get the longest row for comparison, while caching the reads
def readAndGetMaxLength(f):
  global lines

  maxL = i = 0
  for line in f:
    lines.append(line.rstrip())
    if len(lines[i]) > maxL and not isComment(line):
      maxL = len(lines[i])
    i = i + 1
  return maxL

def appendCol(f, max):
  global lines, data

  f.seek(0)
  f.truncate()
  i = 0
  for cell in data:
    if i == 2:
      colName = cell

    if cell == "$" or lines[i] == b"" or isComment(lines[i]):
      f.write(lines[i] + b"\n")
      i = i + 1
      continue

    padding = max - len(lines[i]) + 2 # 2 spaces as field separator
    if i > 2 and "-r" in flags:
      padding = padding  + (len(colName) - len(cell)) # right-align
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

def switchCols(f):
  global lines, data

  f.seek(0)
  f.truncate()
  i = 0
  colIdx1 = 0
  colIdx2 = 0
  for line in lines:
    if i < 2 or line == b"" or isComment(line):
      f.write(line + b"\n")
      i = i + 1
      continue

    if i == 2:
      colNames = line.split()
      col1, col2 = [bytes(datum, "ascii") for datum in data]
      colIdx1 = colNames.index(col1)
      colIdx2 = colNames.index(col2)
    elif i == 3:
      colIdx1 += 1 # skip row names from now on
      colIdx2 += 1

    # lazy approach instead of using spans or format
    lineFields = line.split()
    col1 = lineFields[colIdx1]
    col2 = lineFields[colIdx2]
    lineFields.insert(colIdx1, col2)
    lineFields.pop(colIdx2 + 1)
    lineFields.insert(colIdx2 + 1, col1)
    lineFields.pop(colIdx1 + 1)
    line = b"   ".join(lineFields)
    if i == 2:
      introPad = " " * (len(lines[2]) - len(lines[2].lstrip()))
      f.write((introPad + line.decode() + "\n").encode('ascii'))
    else:
      f.write((line.decode() + "\n").encode('ascii'))

    i = i + 1

################# MAIN ####################################
if len(sys.argv) < 4:
  usage("missing parameters")
  sys.exit(11)

filename = sys.argv[1]
mode = sys.argv[2].upper() # APPEND / APPEND_COL
data = sys.argv[3:][0].split()
flags = sys.argv[4] if len(sys.argv) > 4 else ""

if mode not in ["APPEND", "APPEND_COL", "SWITCH"]:
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
  elif mode == "SWITCH":
    switchCols(f)

# tests
# python admin/extend2da.py gemrb/unhardcoded/bg1/classes.2da APPEND_COL "$ $ BOGOSITY 0 0 0 00 0 0 0 0 0 0 0 0 0 0 2 3 4 7"
# python admin/extend2da.py gemrb/unhardcoded/bg1/classes.2da APPEND "HACKER 1 3 0 0 0 00 0 0 0 0 0 0 0 0 0 0 0 0"
# python admin/extend2da.py gemrb/unhardcoded/bg1/pdolls.2da SWITCH "LEVEL2 SIZE"
