#!/usr/bin/env python3

import struct
import sys

def read32(f): return f.read(4)
def read64(f): return f.read(8)

(TYPE_NOP, TYPE_READ, TYPE_WRITE, TYPE_END) = range(4)
e_types = ["NOP", "READ", "WRITE", "END"]

# main
if len(sys.argv) < 2:
    print("usage: trace_printer <file>")
    exit(1)

infile = open(sys.argv[1], "rb")
infile_type = read32(infile).decode('utf-8')
if infile_type != "4TRF":
    print("trace file format error")
    exit(1)

infile_nprocs = struct.unpack_from(">I", read32(infile))[0]
print(infile_type, infile_nprocs)

while True:
    for p in range(infile_nprocs):
        b = read64(infile)
        if not b:
            break
        n = struct.unpack_from(">Q", b)[0]
        t = n >> 62
        addr = n & ~(3 << 62)
        print("P%d" % p, e_types[t], addr) 
    if not b:
        break
infile.close()
