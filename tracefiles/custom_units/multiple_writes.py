#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 4)

# P0 and P1 write to the same address, racing to get MODIFIED
t.write(0x400)
t.write(0x400)
t.nop()
t.nop()

# P2 and P3 read the same address, should both load from the winner of the previous race and transition to SHARED
t.nop()
t.nop()
t.read(0x400)
t.read(0x400)

# All processors write to the same address again
t.write(0x400)
t.write(0x400)
t.write(0x400)
t.write(0x400)

t.close()