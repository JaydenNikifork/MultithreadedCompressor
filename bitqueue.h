#ifndef BITQUEUE_H
#define BITQUEUE_H

#include <queue>
#include <cstdint>
#include <iostream>
#include <istream>
#include <ostream>


struct BitQueueAccessOverflow {};

class BitQueue {
  std::queue<uint32_t> theQ;

  uint32_t *curWord;
  short int bitNum = 0;

  short int endBitNum = 0;
  short int readBitNum = 0;

public:
  unsigned int size() const;

  void pushBit(int bit);

  void push(uint32_t word);

  void push(uint8_t c);

  void pushWithoutLeading0s(uint32_t word);

  short int front();

  long long readAndPop(unsigned short int numBits);

  void pop();

  bool empty() const;

  friend std::istream &operator>>(std::istream&, BitQueue&);

  friend std::ostream &operator<<(std::ostream&, BitQueue&);
};

unsigned int numBits(uint32_t word);

bool runTests();

#endif
