#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 2)

# 2 processor trace, so generate pairs of events for P0 and P1
t.read(0x20)  # P0: read miss
t.nop()    # P1: Make sure that P0 gets the bus first

t.nop()    # P0:
t.read(0x200) # P1: read unrelated block so P0's read is completed

t.nop()
t.nop()

# Now P0 has 0x20 loaded and cpus should be ready 
t.nop()    # P0:
t.write(0x20) # P1: write miss, will invalidate block in cache P0

# write could allocate first. so delay P0 with dummy read
t.read(0x300)
t.nop()

t.read(0x20)  # P0: block evicted, should miss cache
t.nop()    # P1:

t.close()
