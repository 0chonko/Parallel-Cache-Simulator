#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 2)

# P0 writes to 0x800, should transition to MODIFIED
t.write(0x800)
t.nop()

# P1 reads 0x800, should transition P0 to OWNED and P1 to SHARED
t.nop()
t.read(0x800)

# P0 reads 0x800, should transition from OWNED to SHARED
t.read(0x800)
t.nop()

# Both processors read 0x800 again, should hit in SHARED state
t.read(0x800)
t.read(0x800)

t.close()