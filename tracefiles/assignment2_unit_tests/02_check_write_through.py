#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 2)

# 2 processor trace, so generate pairs of events for P0 and P1
# write some data

# 1
t.read(0x100) # P0:
t.nop()     # P1:

t.write(0x100) # P0:
t.nop()     # P1:

# 2
t.read(0x100) # P0:
t.nop()     # P1:

t.write(0x100) # P0:
t.nop()     # P1:

# 3
t.read(0x100) # P0:
t.nop()     # P1:

t.write(0x100) # P0:
t.nop()     # P1:

# 4
t.read(0x100) # P0:
t.nop()     # P1:

t.write(0x100) # P0:
t.nop()     # P1:

t.close()
