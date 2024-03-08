#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 2)

# 2 processor trace, so generate pairs of events for P0 and P1
# write some data
t.write(0x100)
t.write(0x200)

t.nop()    # P0:
t.nop()    # P1:

t.nop()
t.nop()

t.read(0x100) # if they read hit, we allocate on write.
t.read(0x200)

t.close()
