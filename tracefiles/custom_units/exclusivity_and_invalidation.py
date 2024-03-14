#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 2)

# P0 reads address 0x300, should transition to EXCLUSIVE
t.read(0x300)
t.nop()

# P1 writes to 0x300, should invalidate P0's copy and transition to MODIFIED
t.nop()
t.write(0x300)

# P0 reads 0x300 again, should miss and transition P1's state to OWNED
t.read(0x300)
t.nop()

# P1 writes to 0x300 again, should invalidate P0's copy and transition back to MODIFIED
t.nop()
t.write(0x300)

t.close()