#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 2)

# generate a write race. Should work on both write-allocate and write
# no-allocate.
# Should also work on both MSI and valid/invalid.

# First read block in both caches because we could have a write-no-allocate
# cache.
t.read(0x20)  # P0: read miss
t.read(0x20)   # P1: read miss

# Now let both cpu's write the same block. But make sure P0 wins the race by
# delaying P1.
t.write(0x20) # P0: write miss, will invalidate block in cache 1
t.nop()

# nop nop!
t.nop()
t.nop()

# Time for P1 to write the same block
t.nop()
t.write(0x20) # P1: write miss, will invalidate block in cache 0

# read unrelated block to let the dust settle.
t.read(0x200)
t.read(0x200)
# read unrelated block to let the dust settle.
t.read(0x400)
t.read(0x400)

# Now probe the cache. If all went well, P0's read should miss the cache and
# P1 should hit the cache with a read hit. (if we write-allocate, otherwise
# both caches will miss)
t.read(0x20)
t.read(0x20)

t.close()
