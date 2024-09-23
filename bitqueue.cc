#include <vector>
#include <iostream>
#include <istream>
#include <ostream>
#include "bitqueue.h"

unsigned int BitQueue::size() const { return theQ.size(); }

void BitQueue::pushBit(int bit) {
  if (bitNum == 0) {
    theQ.push(0);
    curWord = &theQ.back();
  }

  *curWord |= (bit & 0x1) << (31 - bitNum);
  
  endBitNum = bitNum;
  bitNum = (bitNum + 1) % 32;
}

void BitQueue::push(uint32_t word) {
  for (int i = 31; i >= 0; --i) {
    pushBit((word & (0x1 << i)) >> i);
  }
}

void BitQueue::push(uint8_t c) {
  for (int i = 7; i >= 0; --i) {
    pushBit((c & (0x1 << i)) >> i);
  }
}

void BitQueue::pushWithoutLeading0s(uint32_t word) {
  short int i = 31;

  while ((word & (0x1 << i)) == 0) --i;

  while (i >= 0) {
    pushBit((word & (0x1 << i)) >> i);
    --i;
  }
}

short int BitQueue::front() {
  return (theQ.front() & (0x1 << (31 - readBitNum))) >> (31 - readBitNum);
}

long long BitQueue::readAndPop(unsigned short int numBits) {
  long long ret = 0;

  while (numBits) {
    ret |= std::move(front()) << (numBits - 1);
    pop();
    --numBits;
  }

  return ret;
}

void BitQueue::pop() {
  if (theQ.empty()) return;

  ++readBitNum;

  if (readBitNum == 32) {
    readBitNum = 0;
    theQ.pop();
  }
}

bool BitQueue::empty() const {
  return theQ.empty() || (theQ.size() == 1 && readBitNum > endBitNum);
}

unsigned int numBits(uint32_t word) {
  unsigned int cnt = 0;
  while (word) {
    ++cnt;
    word >>= 1;
  }
  return cnt;
}

bool runTests() {
  bool result = true;
  
  std::vector<int> intArr = {0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1};
  BitQueue bq;

  for (int &i : intArr) {
    bq.pushBit(i);
  }

  int i = 0;
  while (!bq.empty()) {
    if (i >= intArr.size() || bq.front() != intArr[i]) {
      result = false;
      break;
    }

    bq.pop();
    ++i;
  }
  
  std::cout << "BitQueue test results: " << result << std::endl;

  return result;
}

std::istream &operator>>(std::istream &input, BitQueue &bq) {
  char c;

  input >> c;
  if (!input) return input;

  bq.push(std::move(static_cast<uint8_t>(c)));

  return input;
}

std::ostream &operator<<(std::ostream &output, BitQueue &bq) {
  while (!bq.empty()) {
    output.put(bq.readAndPop(8));
  }

  return output;
}
