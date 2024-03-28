#!/usr/bin/env python3

import struct
import sys
import re

if len(sys.argv) < 2:
    print(f"usage: {sys.argv[0]} <in_file>")
    exit(1)

# This pattern matches atomic trace entries with one or two memory operations:
# eg:
# time     addr1 type op1           addr1 size1  op2           addr2 size2
# 21  4294967296  m   r   139877670233944     4
# 22  4294967296  m   r   139877668087776     4   w   139877668087776 4
pattern = r"(?P<time>\d+)\s+(?P<thread_id>\d+)\s+m\s+(?P<op1>[rw])\s+(?P<addr1>\d+)\s+(?P<size1>\d+)(\s+(?P<op2>[rw])\s+(?P<addr2>\d+)\s+(?P<size2>\d+))?"

infile = open(sys.argv[1], "r")

filtered = 0
for line in infile:
    m = re.search(pattern, line)
    if m:
        warning = ''
        if int(m.group('addr1')) % 4 != 0:
            warning += 'misaligned addr1 '
        if m.group('op2') and (int(m.group('addr2')) % 4 != 0):
            warning += 'misaligned addr2 '
        if len(warning) == 0:
            print(line, end="")
        else:
            filtered += 1
    else:
        print(line, end="")

infile.close()
sys.stderr.write(f"Filtered lines: {filtered}\n")
