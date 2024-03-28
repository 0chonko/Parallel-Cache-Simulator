#!/usr/bin/env python3

import struct
import sys
import re

e_types = ["NOP", "READ", "WRITE", "END"]
NOP = 0
READ = 1
WRITE = 2
END = 3

OP_INDEX = 0
ADDR_INDEX = 1

def write32(f, n): f.write(struct.pack('>I', n))

def write64(f, n): f.write(struct.pack('>Q', n))

def entry(f, op, addr):
    # print(op, addr)
    if op == 'n':
        t = NOP
    elif op == 'r':
        t = READ
    elif op == 'w':
        t = WRITE
    elif op == 'e':
        t = END
    else: 
        raise ValueError('unknown entry type')
    addr = int(addr)
    write64(f, (t << 62) | (addr & ~(3 << 62)))

def get_proc_info(filename):
    proc_mapping = {}
    thread_id = 0
    # thread create trace event
    # time  tid     'tr'
    tr_pattern = r"(?P<time>\d+)\s+(?P<thread_id>\d+)\s+tr"
    with open(filename) as infile:
        for line in infile:
            m = re.search(tr_pattern, line)
            if m:
                proc_mapping[m.group('thread_id')] = thread_id
                thread_id += 1

    return proc_mapping

def append_entry(traces, proc_id, op, addr):
    traces[proc_id].append((op, addr))

# Output traces in the fixed slot pattern
# p0_0, p1_0, .., pn_0, p0_1, p1_1, .., pn_1, ...
def output_traces(traces, outfile):
    nprocs = len(traces)
    trace_lengths = list(map(len, traces))
    ntrace_slots = max(trace_lengths)

    for slot in range(ntrace_slots):
        for proc in range(nprocs):
            if slot < trace_lengths[proc]:
                entry_type = traces[proc][slot][OP_INDEX]
                entry_addr = traces[proc][slot][ADDR_INDEX]
            else:
                entry_type = 'n'
                entry_addr = 0
            entry(outfile, entry_type, entry_addr)

    # Terminate traces.
    for proc in range(nprocs):
        entry(outfile, 'e', 0)

if len(sys.argv) < 3:
    print(f"usage: {sys.argv[0]} <in_file> <out_file>")
    exit(1)

infile_name = sys.argv[1]
outfile_name = sys.argv[2]

proc_mapping = get_proc_info(infile_name)
nprocs = len(proc_mapping)
traces = [[] for _ in range(nprocs)]


# This pattern matches atomic trace entries with one or two memory operations:
# eg:
# time tid     addr1 type op1           addr1 size1  op2           addr2 size2
# 21  4294967296  m   r   139877670233944     4
# 22  4294967296  m   r   139877668087776     4   w   139877668087776 4
pattern = r"(?P<time>\d+)\s+(?P<thread_id>\d+)\s+m\s+(?P<op1>[rw])\s+(?P<addr1>\d+)\s+(?P<size1>\d+)(\s+(?P<op2>[rw])\s+(?P<addr2>\d+)\s+(?P<size2>\d+))?"

infile = open(infile_name, "r")
outfile = open(outfile_name, "wb")
outfile.write(b"4TRF") # trace file signature for 64 bit traces

write32(outfile, nprocs)

for line in infile:
    m = re.search(pattern, line)
    if m:
        proc_id = proc_mapping[m.group('thread_id')]
        append_entry(traces, proc_id, m.group('op1'), m.group('addr1'))
        if m.group('op2'):
            append_entry(traces, proc_id, m.group('op2'), m.group('addr2'))


output_traces(traces, outfile)

infile.close()
outfile.close()
