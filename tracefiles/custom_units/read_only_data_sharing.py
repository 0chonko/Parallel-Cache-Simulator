#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 4)

# All processors read the same address, should all transition to SHARED
t.read(0x200)
t.read(0x200)
t.read(0x200)
t.read(0x200)

# Nops to ensure all reads are completed
t.nop()
t.nop()
t.nop()
t.nop()

# Read again to verify all lines stayed SHARED
t.read(0x200)
t.read(0x200)
t.read(0x200)
t.read(0x200)

t.close()