#ifndef COMPRESS_H
#define COMPRESS_H

#define DEFAULT_CHUNK_SIZE 1024   // should be a power of 2?

#include <istream>
#include <ostream>
#include <queue>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <functional>
#include <iostream>
#include <cstdint>
#include <thread>
#include <future>
#include <mutex>
#include "bitqueue.h"

struct File {
  unsigned int size() const;
  BitQueue data;
};

struct Chunk : public File {
  uint32_t id;
};

class ChunkCompressor {
public:
  virtual Chunk compress(Chunk&) = 0;

  virtual Chunk decompress(Chunk&) = 0;

  virtual ~ChunkCompressor() {};
};

class RLECC : public ChunkCompressor {
public:
  Chunk compress(Chunk&) override;

  Chunk decompress(Chunk&) override;
};

std::unique_ptr<ChunkCompressor> createRLECC();

using CCFactory = std::function<std::unique_ptr<ChunkCompressor>()>;

class Compressor {
  std::unordered_map<std::string, CCFactory> ccs;

protected:
  std::unique_ptr<ChunkCompressor> chunkCompressor;

  std::vector<Chunk> partitionWithoutHeader(std::istream&);

  std::vector<Chunk> partitionWithHeader(std::istream&);

public:
  Compressor(std::string cc = "RLE") {
    ccs["RLE"] = createRLECC;

    chunkCompressor = ccs[cc]();
  }

  virtual File compress(std::istream&) = 0;

  virtual File decompress(std::istream&) = 0;
};

class SinglethreadedCompressor : public Compressor {
protected:
  File compress(std::istream&) override;

  File decompress(std::istream&) override;
};

class MultithreadedCompressor : public Compressor {
protected:
  File compress(std::istream&) override;

  File decompress(std::istream&) override;
};

#endif








