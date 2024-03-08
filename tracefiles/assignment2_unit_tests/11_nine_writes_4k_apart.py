#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 1)

# our lines are 32/0x20 wide, 32kB of 8-way set-associative cache --> 128 sets
# fill set 0 (not present)
for i in range(9): t.write(i * 0x20 * 128)

t.close()

