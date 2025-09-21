/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2025 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "../../plugins/SDLAudio/RingBuffer.h"

#include <gtest/gtest.h>

namespace GemRB {

const static std::vector<char> testData = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };

TEST(RingBufferTest, Fill)
{
	RingBuffer<char> rb { 5 };
	EXPECT_EQ(rb.Fill(testData.data(), 0), size_t(0));
	EXPECT_EQ(rb.Fill(testData.data(), 1), size_t(1));
	EXPECT_EQ(rb.Fill(testData.data(), 1), size_t(1));
	EXPECT_EQ(rb.Fill(testData.data(), 5), size_t(3));
	EXPECT_EQ(rb.Fill(testData.data(), 5), size_t(0));
}

TEST(RingBufferTest, Consume)
{
	RingBuffer<char> rb { 5 };
	std::vector<char> result = { 95, 96, 97, 98, 99 };

	rb.Fill(testData.data(), 5);
	EXPECT_EQ(rb.Consume(result.data(), 0), size_t(0));

	EXPECT_EQ(rb.Consume(result.data(), 1), size_t(1));
	EXPECT_EQ(result[0], 0);
	EXPECT_EQ(result[1], 96);

	EXPECT_EQ(rb.Consume(result.data() + 1, 1), size_t(1));
	EXPECT_EQ(result[1], 1);

	EXPECT_EQ(rb.Consume(result.data() + 2, 5), size_t(3));
	EXPECT_EQ(result[2], 2);
	EXPECT_EQ(result[3], 3);
	EXPECT_EQ(result[4], 4);

	EXPECT_EQ(rb.Consume(result.data() + 5, 5), size_t(0));
}

TEST(RingBufferTest, FillAndConsume)
{
	RingBuffer<char> rb { 5 };
	std::vector<char> result = { 95, 96, 97, 98, 99 };

	// RB: c0, 1, 2, f_, _
	rb.Fill(testData.data(), size_t(3));
	// RB: 0, 1, c2, f_, _
	// R: 0, 1, 97, 98, 99
	rb.Consume(result.data(), size_t(2));

	// RB: 0, 1, c2, 3, f_
	EXPECT_EQ(rb.Fill(testData.data() + 3, 1), size_t(1));
	// RB: 0, 1, 2, 3, cf_
	// R: 0, 1, 2, 3, 99
	rb.Consume(result.data() + 2, size_t(3));

	EXPECT_EQ(result[0], 0);
	EXPECT_EQ(result[1], 1);
	EXPECT_EQ(result[2], 2);
	EXPECT_EQ(result[3], 3);
	EXPECT_EQ(result[4], 99);

	// RB: 5, 6, f2, 3, c4
	EXPECT_EQ(rb.Fill(testData.data() + 4, 3), size_t(3));
	// R: 4, 5, 6, 3, 4
	// RB: 5, 6, cf2, 3, 4
	EXPECT_EQ(rb.Consume(result.data(), 4), size_t(3));
}

TEST(RingBufferTest, Replay)
{
	RingBuffer<char> rb { 65536 * 6 };
	std::vector<char> dummyInput;
	dummyInput.resize(66144);
	std::vector<char> dummyOutput;
	dummyOutput.resize(24576);

	EXPECT_EQ(rb.Consume(dummyOutput.data(), dummyOutput.size()), size_t(0));
	for (uint8_t i = 0; i < 5; ++i) {
		EXPECT_EQ(rb.Fill(dummyInput.data(), dummyInput.size()), dummyInput.size());
	}
	EXPECT_EQ(rb.Fill(dummyInput.data(), dummyInput.size()), size_t(62496));
	EXPECT_EQ(rb.Fill(dummyInput.data(), 3648), size_t(0));

	EXPECT_EQ(rb.Consume(dummyOutput.data(), dummyOutput.size()), dummyOutput.size());

	EXPECT_EQ(rb.Fill(dummyInput.data(), 3648), size_t(3648));
	EXPECT_EQ(rb.Fill(dummyInput.data(), dummyInput.size()), size_t(20928));

	EXPECT_EQ(rb.Consume(dummyOutput.data(), dummyOutput.size()), dummyOutput.size());
}

}
