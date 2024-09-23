#include <sstream>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <memory>
#include "compress.h"


int main(int argc, char *argv[]) {
  if (
      argc != 4 ||
      !(std::string(argv[1]) == "compress" || std::string(argv[1]) == "decompress")
  ) {
    std::cout << "Usage: " << argv[0] << " <\"compress\" | \"decompress\"> <algorithm> <file>" << std::endl;
    return 1;
  }

  //std::unique_ptr<Compressor> compressor = std::make_unique<SinglethreadedCompressor>();
  std::unique_ptr<Compressor> compressor = std::make_unique<MultithreadedCompressor>();
 
  std::ifstream ifile(argv[3]);

  auto start = std::chrono::high_resolution_clock::now();

  File output;
  if (std::string(argv[1]) == "compress") {
    output = compressor->compress(ifile);
  } else {
    output = compressor->decompress(ifile);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  std::cout
    << "Finished "
    << argv[1]
    << " on file "
    << argv[3]
    << " with algorithm "
    << argv[2]
    << " in "
    << duration.count()
    << std::endl;
  
  std::ofstream ofile;

  if (std::string(argv[1]) == "compress") {
    ofile.open(std::string(argv[3]) + "-compressed");
  } else {
    ofile.open(std::string(argv[3]) + "-decompressed");
  }

  ofile << output.data;

  return 0;
}

