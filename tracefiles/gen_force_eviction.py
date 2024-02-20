#!/usr/bin/env python3

import struct
import sys

(TYPE_NOP, TYPE_READ, TYPE_WRITE, TYPE_END) = range(4)

def write32(n): f.write(struct.pack('>I', n))
def write64(n): f.write(struct.pack('>Q', n))

def entry(t, addr): write64((t << 62) | (addr & ~(3 << 62)))

def openTrace(filename, num_proc):
	global f
	f = open(filename, "wb")
	f.write(b"4TRF") # trace file signature
	write32(num_proc)

if len(sys.argv) < 4:
    print("Using cache default config");
    cache_size, block_size, assoc = 32, 32, 8
    offset = 0;
else:
    if len(sys.argv) == 5:
        cache_size, block_size, assoc, offset = map(int, sys.argv[1:])
    else:
        cache_size, block_size, assoc = map(int, sys.argv[1:])
        offset = 0

# calculate cache config
cache_size *= 1024
blocks = cache_size // block_size
sets = blocks // assoc
print(f"cache_size: {cache_size}, block_size: {block_size}, assoc: {assoc}, blocks: {blocks}, sets: {sets}")

openTrace(__file__.replace('.py', '.trf'), 1)

# our lines are 32/0x20 wide, 32kB of 8-way set-associative cache --> 128 sets
# fill the whole set, last write will evict the oldest entry in the set.
for i in range(assoc + 1):
    addr = i * block_size * sets + offset
    print(f"{i}: Writing to address {addr}")
    entry(TYPE_WRITE, addr)

# LRU was evicted, so this read should miss
print(f"Reading from address {offset}, expect a miss due to eviction")
entry(TYPE_READ, offset)

entry(TYPE_END, 0)
f.close()

