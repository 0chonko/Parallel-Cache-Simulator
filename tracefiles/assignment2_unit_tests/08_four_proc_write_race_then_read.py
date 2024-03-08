#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 4)

# four write race.
t.write(0x20) # P0: write 'miss', will invalidate other caches
t.write(0x20) # P0: write 'miss', will invalidate other caches
t.write(0x20) # P0: write 'miss', will invalidate other caches
t.write(0x20) # P0: write 'miss', will invalidate other caches

# 1000 nops for every core
for i in range(1000):
    for j in range(4):
        t.nop()

# four write race.
t.read(0x20)
t.read(0x20)
t.read(0x20)
t.read(0x20)

t.close()
