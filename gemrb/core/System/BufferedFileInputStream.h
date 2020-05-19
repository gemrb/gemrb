#ifndef BUFFERED_FILE_INPUT_STREAM_H
#define BUFFERED_FILE_INPUT_STREAM_H

#include <algorithm>
#include <array>
#include <fstream>

#include "System/DataStream.h"
#include "System/VFS.h"

#define DEFAULT_READ_BUFFER_SIZE 4096

namespace GemRB {

template<size_t BUFFER_SIZE = DEFAULT_READ_BUFFER_SIZE>
class BufferedFileInputStream : public DataStream {
  public:
    BufferedFileInputStream(const std::string& fileName);
    ~BufferedFileInputStream() override {};

    bool isOk() const { return ifstream.is_open(); }
    DataStream* Clone() override {
      return new BufferedFileInputStream{originalfile};
    }
    int Read(void *dest, unsigned int len) override;
    int Seek(int pos, int startPos) override;
    int Write(const void*, unsigned int) override {
      return GEM_ERROR;
    };
  private:
    std::array<char, BUFFER_SIZE> readBuffer;
    uint64_t bufferPos;
    size_t bufferLength;
    bool bufferLoaded;

    std::ifstream ifstream;

    void readToBuffer();
};

template<size_t BS>
BufferedFileInputStream<BS>::BufferedFileInputStream(const std::string& fileName)
  : DataStream(),
    bufferPos(0),
    bufferLength(0),
    bufferLoaded(false)
{
  ifstream.open(fileName, std::ios_base::in | std::ios_base::binary);

  if (isOk()) {
    ifstream.seekg(0, std::ios_base::end);
    this->size = ifstream.tellg();
    ifstream.seekg(0, std::ios_base::beg);

	  strlcpy(originalfile, fileName.c_str(), _MAX_PATH);
  }
}

template<size_t BS>
int BufferedFileInputStream<BS>::Read(void *dest, unsigned int length) {
  if (!isOk()) {
    return GEM_ERROR;
  }

  if (Pos + length > size) {
    return GEM_ERROR;
  }

  if (length == 0) {
    return 0;
  }

  if (!bufferLoaded || Pos < bufferPos || Pos >= bufferPos + bufferLength) {
    readToBuffer();
  }

  size_t bytesToRead = length;
  size_t copyOffset = 0, readOffset = Pos - bufferPos;
  do {
    size_t chunkSize = std::min(bytesToRead, bufferLength - readOffset);

    std::copy(
        std::begin(readBuffer) + readOffset,
        std::begin(readBuffer) + readOffset + chunkSize,
        reinterpret_cast<char*>(dest) + copyOffset
    );

    bytesToRead -= chunkSize;
    copyOffset += chunkSize;
    readOffset = 0;
    this->Pos += chunkSize;

    if (bytesToRead > 0) {
      readToBuffer();
    }
  } while(bytesToRead > 0);

  if (copyOffset != length) {
    return GEM_ERROR;
  }

  if (Encrypted) {
    ReadDecrypted(dest, copyOffset);
  }

  return copyOffset;
}

template<size_t BS>
void BufferedFileInputStream<BS>::readToBuffer() {
  ifstream.seekg(Pos, std::ios_base::beg);
  ifstream.read(readBuffer.data(), BS);
  this->bufferPos = Pos;
  this->bufferLength = ifstream.gcount();
  this->bufferLoaded = true;
}

template<size_t BS>
int BufferedFileInputStream<BS>::Seek(int newPosition, int seekMode) {
  if (!isOk()) {
    return GEM_ERROR;
  }

  switch (seekMode) {
    case GEM_STREAM_END:
      this->Pos = size - newPosition;
      break;
    case GEM_CURRENT_POS:
      this->Pos += newPosition;
      break;
    case GEM_STREAM_START:
      this->Pos = newPosition;
      break;
    default:
      return GEM_ERROR;
  }

  if (Pos >= size) {
    Log(ERROR, "BufferedFileInputStream", "Invalid seek position %ld (limit: %ld)", Pos, size);
    return GEM_ERROR;
  }

  return GEM_OK;
}

}

#endif
