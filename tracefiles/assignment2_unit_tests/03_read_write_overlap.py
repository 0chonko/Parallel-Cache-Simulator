#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 2)

# 2 processor trace, so generate pairs of events for P0 and P1
t.read(0x20)  # P0: read miss
t.nop()

t.nop()
t.write(0x20) # P1: write miss, will invalidate block in cache 0

t.nop()
t.nop()

t.read(0x20) # P0: block evicted, should miss cache
t.nop()

t.close()
