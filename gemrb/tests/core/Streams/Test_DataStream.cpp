/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <gtest/gtest.h>

#include "Streams/FileStream.h"
#include "Streams/MappedFileMemoryStream.h"
#include "Streams/MemoryStream.h"

#include "System/VFS.h"

namespace GemRB {

// GTest does not like testing over abstract DataStream
using DataStreamFactory = std::function<DataStream*(const path_t&)>;

enum class DummyEnum : uint16_t {
	VALUE_1 = 1, VALUE_11 = 11
};

static const path_t READ_TEST_FILE = PathJoin("tests", "resources", "streams", "file_le.bin");
static const path_t DECRYPTION_TEST_FILE = PathJoin("tests", "resources", "streams", "file_encrypted.bin");
static const path_t WRITE_TEST_FILE = PathJoin("tests", "resources", "streams", "write_check_le.bin");

class DataStream_Test : public testing::TestWithParam<DataStreamFactory> {
protected:
	DataStream* stream;
public:
	~DataStream_Test() override {
		delete stream;
	}

	void TearDown() override {
		delete stream;
		this->stream = nullptr;
	}
};

class DataStream_ReadingTest : public DataStream_Test {
	void SetUp() override {
		this->stream = GetParam()(READ_TEST_FILE);
	}
};

class DataStream_DecryptionTest : public DataStream_Test {
	void SetUp() override {
		this->stream = GetParam()(DECRYPTION_TEST_FILE);
	}
};

TEST_P(DataStream_ReadingTest, MetaData) {
	EXPECT_FALSE(stream->CheckEncrypted());
	EXPECT_EQ(stream->GetPos(), 0);
	stream->Seek(1, GEM_STREAM_START);
	EXPECT_EQ(stream->GetPos(), 1);
	stream->Seek(1, GEM_CURRENT_POS);
	EXPECT_EQ(stream->GetPos(), 2);

	stream->Seek(0, GEM_STREAM_END);
	EXPECT_EQ(stream->GetPos(), stream->Size());
}

TEST_P(DataStream_ReadingTest, ReadScalar) {
	uint8_t one;
	EXPECT_EQ(stream->ReadScalar(one), 1);
	EXPECT_EQ(one, 0x01);

	uint16_t two;
	EXPECT_EQ(stream->ReadScalar(two), 2);
	EXPECT_EQ(two, 0x0201);

	uint32_t four;
	auto numBytes = stream->ReadScalar<uint32_t, uint16_t>(four);
	EXPECT_EQ(numBytes, 2);
	EXPECT_EQ(four, 0x0201);
}

TEST_P(DataStream_ReadingTest, ReadEnum) {
	stream->Seek(5, GEM_STREAM_START);
	DummyEnum enumValue;
	EXPECT_EQ(stream->ReadEnum(enumValue), 2);
	EXPECT_EQ(enumValue, DummyEnum::VALUE_11);
}

TEST_P(DataStream_ReadingTest, ReadRTrimString) {
	// cannot test with ordinary strings r/n
	FixedSizeString<10> buffer;
	stream->Seek(7, GEM_STREAM_START);
	EXPECT_EQ(stream->ReadRTrimString(buffer, 10), 10);
	EXPECT_EQ(buffer, "Text text");
}

TEST_P(DataStream_ReadingTest, ReadPoint) {
	Point p;
	stream->Seek(18, GEM_STREAM_START);
	EXPECT_EQ(stream->ReadPoint(p), 4);

	Point expected{0x8, 0x9};
	EXPECT_EQ(p, expected);
}

// equiv. to ReadPoint in terms of reading
TEST_P(DataStream_ReadingTest, ReadSize) {
	Size s;
	stream->Seek(18, GEM_STREAM_START);
	EXPECT_EQ(stream->ReadSize(s), 4);

	Size expected{0x8, 0x9};
	EXPECT_EQ(s, expected);
}

TEST_P(DataStream_ReadingTest, ReadRegion) {
	Region r;
	stream->Seek(18, GEM_STREAM_START);
	EXPECT_EQ(stream->ReadRegion(r), 8);

	Region expected{0x8, 0x9, 0xA, 0xB};
	EXPECT_EQ(r, expected);
}

TEST_P(DataStream_ReadingTest, ReadRegion_AsPoints) {
	Region r;
	stream->Seek(18, GEM_STREAM_START);
	EXPECT_EQ(stream->ReadRegion(r, true), 8);

	Region expected{0x8, 0x9, 0x2, 0x2};
	EXPECT_EQ(r, expected);
}

TEST_P(DataStream_ReadingTest, ReadLine) {
	stream->Seek(26, GEM_STREAM_START);
	std::string buffer{};

	EXPECT_EQ(stream->ReadLine(buffer, 10), 6);
	EXPECT_EQ(buffer, "Line1");
	EXPECT_EQ(stream->ReadLine(buffer, 10), 8);
	EXPECT_EQ(buffer, "Line 2");
	EXPECT_EQ(stream->ReadLine(buffer, 10), 6);
	EXPECT_EQ(buffer, "Line3");
	EXPECT_EQ(stream->ReadLine(buffer, 10), 5);
	EXPECT_EQ(buffer, "Line4");
}

TEST_P(DataStream_DecryptionTest, ReadEncryptedValues) {
	EXPECT_TRUE(stream->CheckEncrypted());

	// sequence of 0 to 63
	for (uint8_t i = 0; i < 64; ++i) {
		uint8_t v;
		stream->ReadScalar(v);
		EXPECT_EQ(v, i);
	}
}

TEST(DataStream_WritingTest, Writes) {
	void *buffer = malloc(35);
	MemoryStream stream{"", buffer, 35};

	EXPECT_EQ(stream.WriteScalar(uint8_t{0x1}), 1);
	EXPECT_EQ(stream.WriteScalar(uint16_t{0x203}), 2);
	auto length = stream.WriteScalar<uint16_t, uint8_t>(uint16_t{0x0});
	EXPECT_EQ(length, 1);
	EXPECT_EQ(stream.WriteEnum(DummyEnum::VALUE_11), 2);
	EXPECT_EQ(stream.WriteFilling(7), 7);

	Point p{0x8, 0x9};
	EXPECT_EQ(stream.WritePoint(p), 4);

	EXPECT_EQ(stream.WriteString(std::string{"String"}, 6), 6);
	EXPECT_EQ(stream.WriteStringLC(std::string{"StRING"}, 6), 6);
	EXPECT_EQ(stream.WriteStringUC(std::string{"sTring"}, 6), 6);
	stream.Rewind();

	FileStream checkStream{};
	checkStream.Open(WRITE_TEST_FILE);

	EXPECT_EQ(checkStream.Size(), stream.Size());

	uint8_t v = 0;
	uint8_t c = 0;
	for (strpos_t i = 0; i < checkStream.Size(); ++i) {
		checkStream.Read(&c, 1);
		stream.Read(&v, 1);
		EXPECT_EQ(v, c);
	}
}

static DataStream* createFileStream(const path_t& path) {
	auto fstream = new FileStream();
	fstream->Open(path);

	return fstream;
}

static DataStream* createMMapStream(const path_t& path) {
	return new MappedFileMemoryStream(path);
}

INSTANTIATE_TEST_SUITE_P(
	DataStreamReadingInstances,
	DataStream_ReadingTest,
	testing::Values(
		DataStreamFactory{createFileStream},
		DataStreamFactory{createMMapStream}
	)
);

INSTANTIATE_TEST_SUITE_P(
	DataStreamDecryptingInstances,
	DataStream_DecryptionTest,
	testing::Values(
		DataStreamFactory{createFileStream},
		DataStreamFactory{createMMapStream}
	)
);

}
