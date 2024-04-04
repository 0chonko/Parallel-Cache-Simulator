#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 3)

# P0 writes to 0x700, should transition to MODIFIED
t.write(0x700)
t.nop()
t.nop()

# P1 reads 0x700, should transition P0 to OWNED and P1 to SHARED
t.nop()
t.read(0x700)
t.nop()

# P2 reads 0x700, should get the data from P0 (OWNED) and transition to SHARED
t.nop()
t.nop()
t.read(0x700)

# P1 writes to 0x700, should invalidate both P0's and P2's copies and transition to MODIFIED
t.nop()
t.write(0x700)
t.nop()

t.close()