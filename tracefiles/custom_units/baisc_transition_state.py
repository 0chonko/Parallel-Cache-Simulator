#!/usr/bin/env python3

from trace_lib import Trace

t = Trace(__file__.replace('.py', '.trf'), 2)

# Processor P0 reads address 0x100, should transition to EXCLUSIVE state
t.read(0x00)
t.nop()

# Processor P1 reads the same address, P0 should transition to SHARED and P1 to SHARED
t.nop()
t.read(0x00)


t.close()
