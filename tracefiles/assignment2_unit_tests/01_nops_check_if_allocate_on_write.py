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

# generate 200 nops to make sure that all load and stores are done
for i in range(200):
    t.nop()
    t.nop()

# for blocking caches (block on write) time should now be ~400

# now the read should hit, so expected time is ~400+

t.read(0x100) # if they read hit, we allocate on write.
t.read(0x200)

t.close()
