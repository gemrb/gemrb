/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

// IP's ACM-stream unpacker.

#include "unpacker.h"

#include <cstdio>

char Table1[27] = {
	0, 1, 2, 4, 5, 6, 8, 9, 10, 16, 17, 18, 20, 21, 22, 24, 25, 26, 32, 33,
	34, 36, 37, 38, 40, 41, 42
};
//Eng: in base-4 system it is:
//Rus: в четверичной системе это будет:
//		000 001 002  010 011 012  020 021 022
//		100 101 102  110 111 112  120 121 122
//		200 201 202  210 211 212  220 221 222
short Table2[125] = {
	0, 1, 2, 3, 4, 8, 9, 10, 11, 12, 16, 17, 18, 19, 20, 24, 25, 26, 27, 28,
	32, 33, 34, 35, 36, 64, 65, 66, 67, 68, 72, 73, 74, 75, 76, 80, 81, 82,
	83, 84, 88, 89, 90, 91, 92, 96, 97, 98, 99, 100, 128, 129, 130, 131, 132,
	136, 137, 138, 139, 140, 144, 145, 146, 147, 148, 152, 153, 154, 155, 156,
	160, 161, 162, 163, 164, 192, 193, 194, 195, 196, 200, 201, 202, 203, 204,
	208, 209, 210, 211, 212, 216, 217, 218, 219, 220, 224, 225, 226, 227, 228,
	256, 257, 258, 259, 260, 264, 265, 266, 267, 268, 272, 273, 274, 275, 276,
	280, 281, 282, 283, 284, 288, 289, 290, 291, 292
};
//Eng: in base-8 system:
//Rus: в восьмеричной системе:
//		000 001 002 003 004  010 011 012 013 014 ...
//		100 101 102 103 104 ...
//		200 ...
//		...
unsigned char Table3[121] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x10,
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x20, 0x21,
	0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x30, 0x31, 0x32,
	0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x40, 0x41, 0x42, 0x43,
	0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x50, 0x51, 0x52, 0x53, 0x54,
	0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65,
	0x66, 0x67, 0x68, 0x69, 0x6A, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
	0x77, 0x78, 0x79, 0x7A, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8A, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9A, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9,
	0xAA
};

FillerProc Fillers[32] = {
	&CValueUnpacker::zero_fill, & CValueUnpacker::return0,
	& CValueUnpacker::return0, & CValueUnpacker::linear_fill,
	& CValueUnpacker::linear_fill, & CValueUnpacker::linear_fill,
	& CValueUnpacker::linear_fill, & CValueUnpacker::linear_fill,
	& CValueUnpacker::linear_fill, & CValueUnpacker::linear_fill,
	& CValueUnpacker::linear_fill, & CValueUnpacker::linear_fill,
	& CValueUnpacker::linear_fill, & CValueUnpacker::linear_fill,
	& CValueUnpacker::linear_fill, & CValueUnpacker::linear_fill,
	& CValueUnpacker::linear_fill, & CValueUnpacker::k1_3bits,
	& CValueUnpacker::k1_2bits, & CValueUnpacker::t1_5bits,
	& CValueUnpacker::k2_4bits, & CValueUnpacker::k2_3bits,
	& CValueUnpacker::t2_7bits, & CValueUnpacker::k3_5bits,
	& CValueUnpacker::k3_4bits, & CValueUnpacker::return0,
	& CValueUnpacker::k4_5bits, & CValueUnpacker::k4_4bits,
	& CValueUnpacker::return0, & CValueUnpacker::t3_7bits,
	& CValueUnpacker::return0, & CValueUnpacker::return0
};

inline void CValueUnpacker::prepare_bits(int bits)
{
	while (bits > avail_bits) {
		unsigned char one_byte;
		if (buffer_bit_offset == UNPACKER_BUFFER_SIZE) {
			unsigned long remains = stream->Remains();
			if (remains > UNPACKER_BUFFER_SIZE)
				remains = UNPACKER_BUFFER_SIZE;
			buffer_bit_offset = UNPACKER_BUFFER_SIZE - remains;
			if (buffer_bit_offset != UNPACKER_BUFFER_SIZE)
				stream->Read( bits_buffer + buffer_bit_offset, remains);
		}
		//our stream read returns -1 instead of 0 on failure
		//comparing with 1 will solve annoying interface changes
		if (buffer_bit_offset < UNPACKER_BUFFER_SIZE) {
			one_byte = bits_buffer[buffer_bit_offset];
			buffer_bit_offset++;
		} else {
			one_byte = 0;
		}
		next_bits |= ( ( unsigned int ) one_byte << avail_bits );
		avail_bits += 8;
	}
}
int CValueUnpacker::get_bits(int bits)
{
	prepare_bits( bits );
	int res = next_bits;
	avail_bits -= bits;
	next_bits >>= bits;
	return res;
}
int CValueUnpacker::init_unpacker()
{
	//using malloc, supposed to be faster
	if (amp_buffer) {
		free(amp_buffer);
	}
	amp_buffer =(short *) malloc(sizeof(short)*0x10000);
	if (!amp_buffer) {
		return 0;
	}
	buff_middle = amp_buffer + 0x8000;
	return 1;
}
int CValueUnpacker::get_one_block(int* block)
{
	block_ptr = block;
	int pwr = get_bits( 4 ) & 0xF, val = get_bits( 16 ) & 0xFFFF,
		count = 1 << pwr, v = 0;
	int i;
	for (i = 0; i < count; i++) {
		buff_middle[i] = ( short ) v;
		v += val;
	}
	v = -val;
	for (i = 0; i < count; i++) {
		buff_middle[-i - 1] = ( short ) v;
		v -= val;
	}

	for (int pass = 0; pass < sb_size; pass++) {
		int ind = get_bits( 5 ) & 0x1F;
		if (!( ( this->*Fillers[ind] ) ( pass, ind ) )) {
			return 0;
		}
	}
	return 1;
}


// Filling functions:
// int CValueUnpacker::FillerProc (int pass, int ind)
int CValueUnpacker::return0(int /*pass*/, int /*ind*/)
{
	return 0;
}
int CValueUnpacker::zero_fill(int pass, int /*ind*/)
{
	//Eng: used when the whole column #pass is zero-filled
	//Rus: используетс€, когда весь столбец с номером pass заполнен нул€ми
	int* sb_ptr = &block_ptr[pass], step = sb_size, i = subblocks;
	do {
		*sb_ptr = 0;
		sb_ptr += step;
	} while (( --i ) != 0);
	return 1;
}

int CValueUnpacker::linear_fill(int pass, int ind)
{
	int mask = ( 1 << ind ) - 1;
	short* lb_ptr = buff_middle + int( ((size_t)-1) << ( ind - 1 ) );

	for (int i = 0; i < subblocks; i++)
		block_ptr[i * sb_size + pass] = lb_ptr[get_bits( ind ) & mask];
	return 1;
}
int CValueUnpacker::k1_3bits(int pass, int /*ind*/)
{
	//Eng: column with number pass is filled with zeros, and also +/-1, zeros are repeated frequently
	//Rus: cтолбец pass заполнен нул€ми, а также +/- 1, но нули часто идут подр€д
	// efficiency (bits per value): 3-p0-2.5*p00, p00 - cnt of paired zeros, p0 - cnt of single zeros.
	//Eng: it makes sense to use, when the freqnecy of paired zeros (p00) is greater than 2/3
	//Rus: имеет смысл использовать, когда веро€тность парных нулей (p00) больше 2/3
	for (int i = 0; i < subblocks; i++) {
		prepare_bits( 3 );
		if (( next_bits & 1 ) == 0) {
			avail_bits--;
			next_bits >>= 1;
			block_ptr[i * sb_size + pass] = 0; if (( ++i ) == subblocks)
												break;
			block_ptr[i * sb_size + pass] = 0;
		} else if (( next_bits & 2 ) == 0) {
			avail_bits -= 2;
			next_bits >>= 2;
			block_ptr[i * sb_size + pass] = 0;
		} else {
			block_ptr[i * sb_size + pass] = ( next_bits & 4 ) ?
				buff_middle[1] :
				buff_middle[-1];
			avail_bits -= 3;
			next_bits >>= 3;
		}
	}
	return 1;
}
int CValueUnpacker::k1_2bits(int pass, int /*ind*/)
{
	//Eng: column is filled with zero and +/-1
	//Rus: cтолбец pass заполнен нул€ми, а также +/- 1
	// efficiency: 2-P0. P0 - cnt of any zero (P0 = p0 + p00)
	//Eng: use it when P0 > 1/3
	//Rus: имеет смысл использовать, когда веро€тность нул€ больше 1/3
	for (int i = 0; i < subblocks; i++) {
		prepare_bits( 2 );
		if (( next_bits & 1 ) == 0) {
			avail_bits--;
			next_bits >>= 1;
			block_ptr[i * sb_size + pass] = 0;
		} else {
			block_ptr[i * sb_size + pass] = ( next_bits & 2 ) ?
				buff_middle[1] :
				buff_middle[-1];
			avail_bits -= 2;
			next_bits >>= 2;
		}
	}
	return 1;
}
int CValueUnpacker::t1_5bits(int pass, int /*ind*/)
{
	//Eng: all the -1, 0, +1 triplets
	//Rus: все комбинации троек -1, 0, +1.
	// efficiency: always 5/3 bits per value
	// use it when P0 <= 1/3
	for (int i = 0; i < subblocks; i++) {
		int bits = ( int ) ( get_bits( 5 ) & 0x1f );
		bits = ( int ) Table1[bits];

		block_ptr[i * sb_size + pass] = buff_middle[-1 + ( bits & 3 )];
		if (( ++i ) == subblocks)
			break;
		bits >>= 2;
		block_ptr[i * sb_size + pass] = buff_middle[-1 + ( bits & 3 )];
		if (( ++i ) == subblocks)
			break;
		bits >>= 2;
		block_ptr[i * sb_size + pass] = buff_middle[-1 + bits];
	}
	return 1;
}
int CValueUnpacker::k2_4bits(int pass, int /*ind*/)
{
	// -2, -1, 0, 1, 2, and repeating zeros
	// efficiency: 4-2*p0-3.5*p00, p00 - cnt of paired zeros, p0 - cnt of single zeros.
	//Eng: makes sense to use when p00>2/3
	//Rus: имеет смысл использовать, когда веро€тность парных нулей (p00) больше 2/3
	for (int i = 0; i < subblocks; i++) {
		prepare_bits( 4 );
		if (( next_bits & 1 ) == 0) {
			avail_bits--;
			next_bits >>= 1;
			block_ptr[i * sb_size + pass] = 0; if (( ++i ) == subblocks)
												break;
			block_ptr[i * sb_size + pass] = 0;
		} else if (( next_bits & 2 ) == 0) {
			avail_bits -= 2;
			next_bits >>= 2;
			block_ptr[i * sb_size + pass] = 0;
		} else {
			block_ptr[i * sb_size + pass] = ( next_bits & 8 ) ?
				( ( next_bits & 4 ) ? buff_middle[2] : buff_middle[1] ) :
				( ( next_bits & 4 ) ? buff_middle[-1] : buff_middle[-2] );
			avail_bits -= 4;
			next_bits >>= 4;
		}
	}
	return 1;
}
int CValueUnpacker::k2_3bits(int pass, int /*ind*/)
{
	// -2, -1, 0, 1, 2
	// efficiency: 3-2*P0, P0 - cnt of any zero (P0 = p0 + p00)
	//Eng: use when P0>1/3
	//Rus: имеет смысл использовать, когда веро€тность нул€ больше 1/3
	for (int i = 0; i < subblocks; i++) {
		prepare_bits( 3 );
		if (( next_bits & 1 ) == 0) {
			avail_bits--;
			next_bits >>= 1;
			block_ptr[i * sb_size + pass] = 0;
		} else {
			block_ptr[i * sb_size + pass] = ( next_bits & 4 ) ?
				( ( next_bits & 2 ) ? buff_middle[2] : buff_middle[1] ) :
				( ( next_bits & 2 ) ? buff_middle[-1] : buff_middle[-2] );
			avail_bits -= 3;
			next_bits >>= 3;
		}
	}
	return 1;
}
int CValueUnpacker::t2_7bits(int pass, int /*ind*/)
{
	//Eng: all the +/-2, +/-1, 0  triplets
	// efficiency: always 7/3 bits per value
	//Rus: все комбинации троек -2, -1, 0, +1, 2.
	// эффективность: 7/3 бита на значение - всегда
	// use it when p0 <= 1/3
	for (int i = 0; i < subblocks; i++) {
		int bits = ( int ) ( get_bits( 7 ) & 0x7f );
		short val = Table2[bits];

		block_ptr[i * sb_size + pass] = buff_middle[-2 + ( val & 7 )];
		if (( ++i ) == subblocks)
			break;
		val >>= 3;
		block_ptr[i * sb_size + pass] = buff_middle[-2 + ( val & 7 )];
		if (( ++i ) == subblocks)
			break;
		val >>= 3;
		block_ptr[i * sb_size + pass] = buff_middle[-2 + val];
	}
	return 1;
}
int CValueUnpacker::k3_5bits(int pass, int /*ind*/)
{
	// fills with values: -3, -2, -1, 0, 1, 2, 3, and double zeros
	// efficiency: 5-3*p0-4.5*p00-p1, p00 - cnt of paired zeros, p0 - cnt of single zeros, p1 - cnt of +/- 1.
	// can be used when frequency of paired zeros (p00) is greater than 2/3
	for (int i = 0; i < subblocks; i++) {
		prepare_bits( 5 );
		if (( next_bits & 1 ) == 0) {
			avail_bits--;
			next_bits >>= 1;
			block_ptr[i * sb_size + pass] = 0; if (( ++i ) == subblocks)
												break;
			block_ptr[i * sb_size + pass] = 0;
		} else if (( next_bits & 2 ) == 0) {
			avail_bits -= 2;
			next_bits >>= 2;
			block_ptr[i * sb_size + pass] = 0;
		} else if (( next_bits & 4 ) == 0) {
			block_ptr[i * sb_size + pass] = ( next_bits & 8 ) ?
					buff_middle[1] :
					buff_middle[-1];
				avail_bits -= 4;
				next_bits >>= 4;
		} else {
			avail_bits -= 5;
			int val = ( next_bits & 0x18 ) >> 3;
			next_bits >>= 5;
			if (val >= 2)
				val += 3;
			block_ptr[i * sb_size + pass] = buff_middle[-3 + val];
		}
	}
	return 1;
}
int CValueUnpacker::k3_4bits(int pass, int /*ind*/)
{
	// fills with values: -3, -2, -1, 0, 1, 2, 3.
	// efficiency: 4-3*P0-p1, P0 - cnt of all zeros (P0 = p0 + p00), p1 - cnt of +/- 1.
	for (int i = 0; i < subblocks; i++) {
		prepare_bits( 4 );
		if (( next_bits & 1 ) == 0) {
			avail_bits--;
			next_bits >>= 1;
			block_ptr[i * sb_size + pass] = 0;
		} else if (( next_bits & 2 ) == 0) {
			avail_bits -= 3;
			block_ptr[i * sb_size + pass] = ( next_bits & 4 ) ?
				buff_middle[1] :
				buff_middle[-1];
			next_bits >>= 3;
		} else {
			int val = ( next_bits &0xC ) >> 2;
			avail_bits -= 4;
			next_bits >>= 4;
			if (val >= 2)
				val += 3;
			block_ptr[i * sb_size + pass] = buff_middle[-3 + val];
		}
	}
	return 1;
}
int CValueUnpacker::k4_5bits(int pass, int /*ind*/)
{
	// fills with values: +/-4, +/-3, +/-2, +/-1, 0, and double zeros
	// efficiency: 5-3*p0-4.5*p00, p00 - cnt of paired zeros, p0 - cnt of single zeros.
	//Eng: makes sense to use when p00>2/3
	//Rus: имеет смысл использовать, когда веро€тность парных нулей (p00) больше 2/3
	for (int i = 0; i < subblocks; i++) {
		prepare_bits( 5 );
		if (( next_bits & 1 ) == 0) {
			avail_bits--;
			next_bits >>= 1;
			block_ptr[i * sb_size + pass] = 0; if (( ++i ) == subblocks)
												break;
			block_ptr[i * sb_size + pass] = 0;
		} else if (( next_bits & 2 ) == 0) {
			avail_bits -= 2;
			next_bits >>= 2;
			block_ptr[i * sb_size + pass] = 0;
		} else {
			int val = ( next_bits &0x1C ) >> 2;
			if (val >= 4)
				val++;
			block_ptr[i * sb_size + pass] = buff_middle[-4 + val];
			avail_bits -= 5;
			next_bits >>= 5;
		}
	}
	return 1;
}
int CValueUnpacker::k4_4bits(int pass, int /*ind*/)
{
	// fills with values: +/-4, +/-3, +/-2, +/-1, 0, and double zeros
	// efficiency: 4-3*P0, P0 - cnt of all zeros (both single and paired).
	for (int i = 0; i < subblocks; i++) {
		prepare_bits( 4 );
		if (( next_bits & 1 ) == 0) {
			avail_bits--;
			next_bits >>= 1;
			block_ptr[i * sb_size + pass] = 0;
		} else {
			int val = ( next_bits &0xE ) >> 1;
			avail_bits -= 4;
			next_bits >>= 4;
			if (val >= 4)
				val++;
			block_ptr[i * sb_size + pass] = buff_middle[-4 + val];
		}
	}
	return 1;
}
int CValueUnpacker::t3_7bits(int pass, int /*ind*/)
{
	//Eng: all the pairs of values from -5 to +5
	// efficiency: 7/2 bits per value
	//Rus: все комбинации пар от -5 до +5
	// эффективность: 7/2 бита на значение - всегда
	for (int i = 0; i < subblocks; i++) {
		int bits = ( int ) ( get_bits( 7 ) & 0x7f );
		unsigned char val = Table3[bits];

		block_ptr[i * sb_size + pass] = buff_middle[-5 + ( val & 0xF )];
		if (( ++i ) == subblocks)
			break;
		val >>= 4;
		block_ptr[i * sb_size + pass] = buff_middle[-5 + val];
	}
	return 1;
}
