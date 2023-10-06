#!/usr/bin/env python3

# Helper script for encrypting asset files using the IE key.
# Useful for testing purposes.
#
# $ python encrypt.py input output

import sys

KEY = b'\x88\xA8\x8F\xBA\x8A\xD3\xB9\xF5\xED\xB1\xCF\xEA\xAA\xE4\xB5\xFB\xEB\x82\xF9\x90\xCA\xC9\xB5\xE7\xDC\x8E\xB7\xAC\xEE\xF7\xE0\xCA\x8E\xEA\xCA\x80\xCE\xC5\xAD\xB7\xC4\xD0\x84\x93\xD5\xF0\xEB\xC8\xB4\x9D\xCC\xAF\xA5\x95\xBA\x99\x87\xD2\x9D\xE3\x91\xBA\x90\xCA'
CHUNK_SIZE = 4096

if len(sys.argv) < 3:
	print('Please specify one input and one output path', file=sys.stderr)
	exit(1)

with open(sys.argv[1], 'rb') as f_in:
	with open(sys.argv[2], 'wb') as f_out:
		f_out.write(b'\xFF\xFF')

		while True:
			chunk = bytearray(f_in.read(CHUNK_SIZE))
			chunk_size = len(chunk)

			if chunk_size == 0:
				break

			for i in range(0, chunk_size):
				chunk[i] = chunk[i] ^ KEY[i % 64]

			f_out.write(chunk)

