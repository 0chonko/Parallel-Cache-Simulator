#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 2)

# P0 reads 0x900, should transition to EXCLUSIVE
t.read(0x900)
t.nop()

# P0 writes to 0x900, should transition from EXCLUSIVE to MODIFIED
t.write(0x900)
t.nop()

# P1 reads 0x900, should miss and transition P0 from MODIFIED to OWNED
t.nop()
t.read(0x900)

# P0 writes to 0x900 again, should invalidate P1's copy and transition back to MODIFIED
t.write(0x900)
t.nop()

t.close()