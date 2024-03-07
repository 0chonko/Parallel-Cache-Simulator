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

openTrace(__file__.replace('.py', '.trf'), 1)

# Write.
entry(TYPE_WRITE, 0x100)

# Do nothing.
entry(TYPE_NOP, 0x0)

# Do nothing.
entry(TYPE_NOP, 0x0)

# We allocate on write so this read should hit.
entry(TYPE_READ, 0x100)

# For a writeback cache this write on the *same cache line* should hit and taken 1 cycle.
entry(TYPE_WRITE, 0x104)

# End of trace
entry(TYPE_END, 0x0)

f.close()
