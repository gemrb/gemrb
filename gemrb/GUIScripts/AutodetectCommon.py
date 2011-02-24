# -*-python-*-
# vim: set ts=4 sw=4 expandtab:

import os
import GemRB


fdict = {}

# Create dict of files in GamePath and GamePath/data
for file in os.listdir (GemRB.GamePath):
    ufile = file.upper()
    if ufile == 'DATA':
        for file2 in os.listdir (os.path.join(GemRB.GamePath, file)):
            fdict[file2.upper()] = 1
    else:
        fdict[ufile] = 1


#
# Return True if all given files/resrefs exist
#
# FILES is a list of tuples NAME, EXTENSION, TYPE
#

def CheckFiles(files):
    for name, ext, type in files:
        res = (name+'.'+ext).upper() in fdict or GemRB.HasResource (name, type)
        #print name+'.'+ext, res
        if not res:
            return False

    return True

