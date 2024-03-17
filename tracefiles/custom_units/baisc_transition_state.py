#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 2)

# Processor P0 reads address 0x100, should transition to EXCLUSIVE state
t.read(0x20)
t.nop()

# Processor P1 reads the same address, P0 should transition to SHARED and P1 to SHARED
t.nop()
t.read(0x24)

# P0 writes to address 0x100, should invalidate the line in P1's cache and transition to MODIFIED
t.write(0x28)
t.nop()

# P1 reads 0x100, should miss and transition P0's state to OWNED
t.nop()
t.read(0x30)

t.close()
