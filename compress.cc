#include "compress.h"

unsigned int File::size() const { return data.size(); }

Chunk RLECC::compress(Chunk &chunk) {
  short int curRun = -1;
  unsigned int runLen = 0;
  short int curBit = -1;
  Chunk outputChunk;
  outputChunk.id = chunk.id;

  // while (!/*doneReadingInput*/) {
  while (!chunk.data.empty()) {
    // WAIT FOR QUEUE TO HAVE ITEM
  
    curBit = std::move(chunk.data.front());
    chunk.data.pop();
    
    if (curRun == -1) {
      curRun = curBit;
      outputChunk.data.pushBit(curBit);
    } else if (curBit != curRun) {
      unsigned int runLenBitsNum = numBits(runLen) - 1;
      for (int i = 0; i < runLenBitsNum; ++i) outputChunk.data.pushBit(0);
      outputChunk.data.pushWithoutLeading0s(runLen);
      curRun = curBit;
      runLen = 0;
    }

    ++runLen;
  }

  if (curRun != -1) {
    unsigned int runLenBitsNum = numBits(runLen) - 1;
    for (int i = 0; i < runLenBitsNum; ++i) outputChunk.data.pushBit(0);
    outputChunk.data.pushWithoutLeading0s(runLen);
  }

  return outputChunk;
}

Chunk RLECC::decompress(Chunk &chunk) {
  short int curRun = std::move(chunk.data.front());
  unsigned int runLenBitsNum = 1;
  unsigned int runLen = 0;
  Chunk outputChunk;
  outputChunk.id = chunk.id;
  chunk.data.pop();

  while (!chunk.data.empty()) {
    while (!chunk.data.empty() && chunk.data.front() == 0) {
      ++runLenBitsNum;
      chunk.data.pop();
    }

    if (chunk.data.empty()) return outputChunk;  // the file will have 31 or less 0 bits tailing
    while (runLenBitsNum) {
      runLen <<= 1;
      runLen += std::move(chunk.data.front());
      chunk.data.pop();
      --runLenBitsNum;
    }

    while (runLen) {
      outputChunk.data.pushBit(curRun);
      --runLen;
    }

    runLenBitsNum = 1;
    runLen = 0;
    curRun = !curRun;
  }

  return outputChunk;
}

std::vector<Chunk> Compressor::partitionWithoutHeader(std::istream &input) {
  char c;
  uint32_t idCount = 0;
  std::vector<Chunk> chunks;

  while (input.get(c)) {
    if (!chunks.size() || chunks.back().size() == DEFAULT_CHUNK_SIZE) {
      chunks.push_back(Chunk{});
      chunks.back().id = idCount++;
    }
    chunks.back().data.push(static_cast<uint8_t>(c));
  }

  return chunks;
}

std::vector<Chunk> Compressor::partitionWithHeader(std::istream &input) {
  std::vector<Chunk> chunks;

  while (input) {
    uint32_t id = 0;
    for (short i = 3; i >= 0; --i) {
      id |= input.get() << (i * 8);
    }

    uint32_t size = 0;
    for (short i = 3; i >= 0; --i) {
      size |= static_cast<uint8_t>(input.get()) << (i * 8);
    }
    if (static_cast<int>(size) == -1) return chunks;

    chunks.push_back(Chunk{});
    chunks.back().id = id;

    short byteCount = 0;
    while (chunks.back().size() < size || byteCount) {
      chunks.back().data.push(static_cast<uint8_t>(input.get()));
      byteCount = (byteCount + 1) % 4;
    }
  }

  return chunks;
}

File SinglethreadedCompressor::compress(std::istream &input) {
  std::vector<Chunk> chunks = partitionWithoutHeader(input);

  File file;

  for (Chunk &chunk : chunks) {
    Chunk compressedChunk = chunkCompressor->compress(chunk);
    file.data.push(static_cast<uint32_t>(compressedChunk.id));
    file.data.push(static_cast<uint32_t>(compressedChunk.size()));
    short bitCount = 0;
    while (!compressedChunk.data.empty()) {
      file.data.pushBit(compressedChunk.data.front());
      compressedChunk.data.pop();
      bitCount = (bitCount + 1) % 32;
    }

    while (bitCount) {
      file.data.pushBit(0);
      bitCount = (bitCount + 1) % 32;
    }
  }

  return file;
}

File SinglethreadedCompressor::decompress(std::istream &input) {
  std::vector<Chunk> chunks = partitionWithHeader(input);

  File file;

  for (Chunk &chunk : chunks) {
    Chunk compressedChunk = chunkCompressor->decompress(chunk);
    while (!compressedChunk.data.empty()) {
      file.data.pushBit(compressedChunk.data.front());
      compressedChunk.data.pop();
    }
  }

  return file;
}

std::unique_ptr<ChunkCompressor> createRLECC() {
  return std::make_unique<RLECC>();
}

File MultithreadedCompressor::compress(std::istream &input) {
  std::vector<Chunk> chunks = partitionWithoutHeader(input);
  std::vector<std::thread> threads;
  threads.reserve(chunks.size());

  std::mutex fileLock;
  File file;

  for (Chunk &chunk : chunks) {
    std::thread t([&](){
      Chunk compressedChunk = chunkCompressor->compress(chunk);

      fileLock.lock();
      file.data.push(static_cast<uint32_t>(compressedChunk.id));
      file.data.push(static_cast<uint32_t>(compressedChunk.size()));
      short bitCount = 0;
      while (!compressedChunk.data.empty()) {
        file.data.pushBit(compressedChunk.data.front());
        compressedChunk.data.pop();
        bitCount = (bitCount + 1) % 32;
      }

      while (bitCount) {
        file.data.pushBit(0);
        bitCount = (bitCount + 1) % 32;
      }
      fileLock.unlock();
    });
    threads.push_back(std::move(t));
  }

  for (std::thread &t : threads) t.join();

  return file;
}

File MultithreadedCompressor::decompress(std::istream &input) {
  std::vector<Chunk> chunks = partitionWithHeader(input);
  std::vector<std::promise<void>> promises(chunks.size());
  std::vector<std::thread> threads;
  threads.reserve(chunks.size());

  File file;

  for (Chunk &chunk : chunks) {
    std::thread t([&](){
      Chunk compressedChunk = chunkCompressor->decompress(chunk);
      if (compressedChunk.id) promises[compressedChunk.id - 1].get_future().wait();
      while (!compressedChunk.data.empty()) {
        file.data.pushBit(compressedChunk.data.front());
        compressedChunk.data.pop();
      }
      promises[compressedChunk.id].set_value();
    });
    threads.push_back(std::move(t));
  }

  for (std::thread &t : threads) t.join();

  return file;
}
