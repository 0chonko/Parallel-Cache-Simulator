#!/usr/bin/env python3

import struct

class Trace:
    (TYPE_NOP, TYPE_READ, TYPE_WRITE, TYPE_END) = range(4)

    def __init__(self, filename, num_proc):
        self.num_proc = num_proc
        self.f = open(filename, "wb")
        self.f.write(b"4TRF") # trace file signature
        self.write32(self.num_proc)

    def write32(self, n):
        self.f.write(struct.pack('>I', n))

    def write64(self, n):
        self.f.write(struct.pack('>Q', n))

    def entry(self, t, addr):
        self.write64((t << 62) | (addr & ~(3 << 62)))

    def read(self, addr):
        self.entry(self.TYPE_READ, addr)

    def write(self, addr):
        self.entry(self.TYPE_WRITE, addr)

    def nop(self):
        self.entry(self.TYPE_NOP, 0x0)

    def close(self):
        for _ in range(self.num_proc):
            self.entry(self.TYPE_END, 0x0)
        self.f.close()

# Open a trace for a 2 processor system.
t = Trace(__file__.replace('.py', '.trf'), 2)

# P0 reads, P1 does nothing.
t.read(0x20)    # P0
t.nop()         # P1

# wait for 150 cycles
for _ in range(150):
    t.nop()     # P0
    t.nop()     # P1

# P0 does nothing, P1 writes and invalidates the line in P0
t.nop()         # P0
t.write(0x20)   # P1

# wait for 150 cycles
for _ in range(150):
    t.nop()     # P0
    t.nop()     # P1

# P0 read should now miss, P1 read should hit.
t.read(0x20)    # P0
t.read(0x20)    # P1

t.close()

