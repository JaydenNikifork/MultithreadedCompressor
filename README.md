# Multithreaded (De)Compressor

### Summary

Here we have a modular, multithreaded file compressor written in C++. 
It works by partitioning files into chunks and compressing/decompressing
them individually on separate threads.

### Compression Algorithms

Currently this compressor only supports run-length encoding (RLE) 
compression (which in hindsight is a terrible algorithm and just 
makes my files bigger...). The compressor provides the algorithm 
the binary of the file, and RLE is run over that. For instance,
rather than AAAB becoming 3A1B, we instead encoding the binary
runs of AAAB (which I will not write out!).

### Usage

Compile with (or your C++ compiler of choice):
```
g++ main.cc compress.cc bitqueue.cc -o compressor
```
Then run:
```
./compressor <"compress" | "decompress"> <algorithm> <file>
```
`<"compress" | "decompress">` to either compress or decompress, 
`<algorithm>` to select which compression algo to use (RLE only for now), 
and `<file>` as the name of your file to run on. When "compress" is 
selected, the generated file will be named "<file>-comressed". When 
"decompress" is selected, the generated file will be named 
"<file>-decompressed".

### Deeper Dive

##### Compression

Compression and decompression is done over `istream`s. As we read from 
the stream (during compression), we partition the data into fixed-sized
Chunks. Chunks have an id, size, and data. The id is used to determine 
the write order during decompression (more on this later). Data and size 
are of course the data from the `istream` and the size of data in bytes. 
The task of partitioning is run over a single thread. After partitioning, 
we begin compression on each of the Chunks. Each compression algo is run 
on a separate thread and writes to the target file immediate on the 
threads completion, unless another thread is current writing, which
in that case it waits. We handle this waiting using a mutex lock. 
This method, results in Chunks being written out of order and 
compression being non-deterministic. Not to worry, we write a short header 
for each Chunk with it's id and size which will help us during decompression.

##### Decompression

Decompression works the same way as compression, apart from a few key 
differences. Firstly, we use a different file partitioning algorithm. 
Since the compressed file was written with compressed Chunks, we cannot 
use the same fixed-sized Chunk partitioning. Now, we use the fixed-sized
headers we wrote for each Chunk to determine how much text to read per 
Chunk. After we partition, we decompress each Chunk on it's own thread.
Difference 2 is that we do not attempt to write to output immediately 
after decompression. We first must ensure the previous Chunk has been 
written already so we obtain the same Chunk ordering as the original file. 
The define the previous Chunk to be the Chunk with id = current Chunk.id - 
1\. We then define the first Chunk (the one with no previous) to have id = 0.
On the implementation side, I used promises and futures for awaiting.

##### String to Bits

Handling strings (not actually strings, but rather a stream of chars) by 
using their binary representation is a pain. More specifically, what I mean 
is the actual labour of extracting the individual bits from chars so we can
run compression over them. My solution to this was to abstract this all away 
into a data structure I called the BitQueue. A BitQueue essentially just 
functions as a queue of bits. It has utilities such as multiple overrides of 
push, so we can push 1 bit, 8 bits, or 32 bits at a time. We can also push 
binary representation of numbers without any leading 0s. We can read and pop 
X number of bits in one go. So on and so forth. The naive solution to 
implementing BitQueue is to use a queue of booleans. Nothing incorrect about 
this, but each boolean is 1 byte of space, and each bit really should only 
use 1 bit of memory, we don't want to x8 our mem usage. Hence, I instead 
used a queue of `uint32_t`, which x32's our mem usage! Just kidding of 
course, what I did was simply store the bits within the bits of the 
`uint32_t`s, so no memory is gone to waste. Now we can easily read the 
binary of our string by pushing each char (8 bits) to our BitQueue, 
then reading them off 1 by 1.

### Hindsight

- We waste time reading an entire string into the BitQueue only then to 
just output some bits. Rather than reading into a BitQueue, a BitQueue 
should sit on top of a string and read over it, rather than doing any 
storage of bits at all. 
- I would like to look into if reading a file can be done over multiple 
threads. That is, each thread starts at some offset from the beginning 
of the file, then reads a Chunk. I would assume that partitioning in 
theory takes around 50% of the computation time since parition and RLE are
both O(n).
- I should be running (de)compression immediately as Chunks are created, 
but I currently wait for the entire file to be read and partitioned first.
- I would like to implement another compression algorithm that works better 
over text. 
- I tried to make good use of move constructors and the move assignment 
operators to reduce copying, but I could have done better in some areas.
- Currently we only have methods to run (de)compression over `istream`s, 
but we should be able to do it over more data types. Another layer of 
abstraction between input and the `Compressor` class might make that cleaner.
- I think the current class structure is okay, but something feels a bit 
off about it for some reason. Would like to clean that up.
