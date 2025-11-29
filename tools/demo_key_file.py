#!/usr/bin/env python3

# Helper script to scan for all the resources of the demo,
# and to create a chitin.key file from it. While this isn't
# strictly necessary for GemRB, other tools may depend on this,
# like NearInfinity.
#
# Use like this to update demo/chitin.key from the project's root:
#
# $ python tools/demo_key_file.py demo

from glob import glob
import os
import sys
from struct import pack

KEY_FILE_NAME = 'chitin.key'

# subdirectories must be set explicitly
SCAN_DIRECTORIES = ['override']

# only what fits into 2 bytes
FILE_EXT_MAPPING = {
	'2da':  0x3F4,
	'acm':  0xFFF,
	'are':  0x3F2,
	'bam':  0x3E8,
	'bcs':  0x3EF,
	'bs':   0x3E9,
	'bmp':    0x1,
	'chr':  0x3FA,
	'chu':  0x3EA,
	'cre':  0x3F1,
	'dlg':  0x3F3,
	'eff':  0x3F8,
	'fnt':  0x400,
	'gam':  0x3F5,
	'glsl': 0x3F5,
	'gui':  0x402,
	'ids':  0x3F0,
	'ini':  0x802,
	'itm':  0x3ED,
	'mos':  0x3EC,
	'mus':  0xFFE,
	'mve':    0x2,
	'ogg':    0x7,
	'plt':    0x6,
	'pro':  0x3FD,
	'pvrz': 0x404,
	'png':  0x40B,
	'res':  0x3FE,
	'spl':  0x3EE,
	'sto':  0x3F6,
	'tis':  0x3EB,
	'toh':  0x407,
	'tot':  0x406,
	'vef':  0x3FC,
	'vvc':  0x3FB,
	'wav':    0x4,
	'wbm':  0x3FF,
	'wed':  0x3E9,
	'wfx':    0x5,
	'wmp':  0x3F7,
}

def create_key_index(root_path):
	index = []

	for directory in SCAN_DIRECTORIES:
		scan_path = os.path.join(root_path, directory)

		for file_path in glob(os.path.join(scan_path, '**')):
			if os.path.isdir(file_path):
				continue

			file_name = os.path.basename(file_path)
			file_ext = os.path.splitext(file_name)

			if not file_ext[1]:
				print(f"Ignoring {file_name} for missing file extension")
				continue

			if len(file_ext[0].encode('ascii')) > 8:
				print(f"Ignoring {file_name} for name being too long")
				continue

			try:
				type_id = FILE_EXT_MAPPING[file_ext[1].lower()[1:]]
			except KeyError:
				print(f"Ignoring {file_name} for unsupported file extension")
				continue

			index.append((file_ext[0], type_id))

	return index

def create_key_file(root_path, index):
	key_file_path = os.path.join(root_path, KEY_FILE_NAME)
	with open(key_file_path, 'wb') as f:
		f.write(b'KEY V1  ')
		# # bif entries
		f.write(pack('i', 0))
		# # items
		f.write(pack('i', len(index)))

		# bif offset (none)
		f.write(pack('i', 0))
		# items offset
		f.write(pack('i', 24))

		# items
		for item in index:
			filename_bytes = item[0].encode('ascii')
			f.write(filename_bytes)

			name_length = len(filename_bytes)
			for _ in range(0, 8 - name_length):
				f.write(b'\x00')

			f.write(pack('h', item[1]))
			f.write(pack('i', 0))

if len(sys.argv) < 2:
	print("Please specify the demo's root folder path.", file=sys.stderr)
	exit(1)

root_path = sys.argv[1]

if not os.path.exists(root_path):
	print("The given path does not exist.", file=sys.stderr)
	exit(1)

index = create_key_index(root_path)
create_key_file(root_path, index)
