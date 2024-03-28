atomic-memory-trace
===================

PIN-tool to produce multi-threaded atomic memory traces

PIN is a useful tool for instrumenting applications and easily producing memory
access traces.  However, tracing memory accesses from multiple threads suffers
from the atomic instrumentation problem -- instructions responsible for 
tracing/logging an access happen separately from that access.  Races between
threads may result in a different order being traced than actually occurs.
This tool provides atomic instrumentation by simulating cache coherence.  In
addition, the tool will trace thread start/end, an optional region of interest,
and user-provided fuction calls and returns.

The primary alternative to this tool is architectural simulation.  Most
simulators are complicated to learn, complicated to use (getting OSes and 
workloads running properly may be difficult), and slow (most simulators are 
single threaded and cannot leverage multithreading to produce a faster
trace/simulation).

This README documents the tracing pintool and example test case.  This tool was 
developed using verion 2.12-58423 of PIN on Ubuntu 12.04.  There are no plans
to support operating systems other than Linux or systems other than x86_64.
The pintool relies on the boots libraries.  This software comes with no support
but may be useful to others.  This project uses the MIT license.

Quick Start
===========

build the pintool
Change into the trace directory

```
% make PIN_ROOT=<your pin root>
```

Run the tool as any other pintool:

```
% pin -t trace/obj-intel64/trace.so -- <your program>
```

By default, output appears in the file memory_trace.out.  All threads and 
memory accesses will be traced.  The output appears with one event (thread
start, function call, or memory access) per line, starting with an arbitrary
timestamp (used to merge events later.

Any easy way to produce useful output, sorting by timestamp and then stripping
timestamps away is to use:

```
% sort -k 1 -n memory_trace.out | sed '/thread_sync/d' | awk 'BEGIN {OFS="\t"}; {$1="";sub("\t\t","")}1' > memory_trace.clean
```

memory_trace.clean will contain properly ordered events and accesses and remove
sync entries

Tool Options
============

* -o  
    The output file name.  By default is 'memory_trace.out'
* -r  
    Do threads need to be registered?  If 0/false all memory accesses from all
    threads will be traced.  If 1/true only accesses from registered threads will
    be traced.  See annotation's atomic_trace::register_thread(threadid).
* -f  
    File with list of functions to trace
* -i  
    Use Region of Interest?  If ROI is used memory tracing will only occur while
    the ROI is active.  Thread start/stop tracing and function tracing will
    always occur.
* -l  
    Number of locks for simulated cache coherence.  Increasing this number will
    use more memory and may hurt cache performance but will improve concurrency.
    If contention occurs for specific address locks (i.e. cache lines) try
    increasing this.  A value of 1 serializes all memory accesses across threads.
* -b  
    Cache block size.  By default 64 bytes.
* -a  
    Accesses per thread before flushing.  Each thread keeps a local trace buffer
    that is occasionally flushed to the global file.  More accesses per thread
    ensures that threads grabbing the global lock does not become the primary
    bottleneck.
* -t  
    Test.  Turns off address locking, breaking atomicity.  Activate this flag to
    see the instrumentation atomicity problem.
* -d  
    Allowable timestamp difference.  If threads diverge in timestamps by beyond
    this limit threads will synch and flush other threads.  This makes merging
    the output significantly easier.
* -c
    Trace Failed Compare-And-Swaps.  Default 0 (no).  Generally every CAS
    instruction is considered a write, even when the instruction fails.
    Use this option to only log CAS as writes when they succeed.

Output Format
=============

Each line contains one event as a tab delimited list.  Entries contain
threadids which may be -1 (if threads must be registered but a traced function
is called from an unregistered thread), assigned by the pintool if threads are
not required to be registered, or set by the registration function (described
later).  All entries start with a timestamp and threadid:

* memory: Each instruction may read two addresses and write one.  There are
possible sub-entries for each of these accesses.  The second read does not
contain a size field as it may only occur with the first read and have the same
size (that is, r2's size is the same as r's).

```
<timestamp> <thread> m [r <address> <size>] [r2 <address>] [w <address> <size>]
```

* thread registered:

```
<timestamp> threadid tr
```

* thread finished:

```
<timestamp> threadid tf
```

* function call: All requested functions are traced, even if not on a registered
thread.  The first 3 arguments of the function are traced as well as the stack
pointer to match up calls and returns

```
<timestamp> <threadid> fc <function name> <function stack pointer> <first arg> <second arg> <third arg>
```

* function return:

```
<timestamp> <threadid> fr <function name> <function stack pointer> <return value>
```

* start Region of Interest:

```
<timestamp> <threadid> start_roi
```

* end Region of Interest:

```
<timestamp> <threadid> end_roi
```

* context change: Context changes may interrupt the locking necessary to provide
atomic tracing.  On a context change consider that the next access may not be
traced atomically.  -- It is unclear how the PIN internals work and if this is
really a concern (I haven't observed any context changes yet).

```
<timestamp> <threadid> ctxt_change
```

* thread sync: When threads flush other threads to keep all threads close in
timestamps the merging process must be made aware of this.

```
<timestamp> <threadid> thread_sync
```

Function Tracing
================

In addition to memory accesses many functions are traced.  A few are specific
to this tool, but any user-provided function can be traced.  The provided
src/annotation.cpp and src/annotation.h (creates libannotation) provides
header and library for these functions.  In general it is easier to provide
these as a library to ensure they are not in-lined.

```
atomic_trace::register_thread
atomic_trace::start_roi
atomic_trace::end_roi
```

These functions allow the pintool to highlight a region of interest (memory
accesses outside of the region will not be traced) and name threads, useful to
match trace threads to user threads.

In addition, the pintool takes an "-f" argument that is a file with a list of
functions (one per line) that will be traced.  The functions should be listed
in their undecorated form (as per pin, see above for examples).
