#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 2)

# Must be allocate on write
t.write(0x20)  # P0: read miss
t.write(0x20)   # P1: read miss

t.close()
