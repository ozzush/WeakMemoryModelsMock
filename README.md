# Weak Memory Models Mock

Supports SC, TSO, PSO and RA/SRA (with RA and SC fences) memory models. Has two
execution modes: random and interactive.

## Architecture

### Program

This module parses the input file and returns a set of corresponding
programs

### Execution

This module takes a set of programs and a memory model
implementation, assigns a thread to each program and executes these threads step
by step. In each step the executor can either execute a command from one of the
threads or give the memory subsystem a command to perform some internal update.
The exact steps to perform depend on the memory subsystem.

### Storage

Storage managers implemented in this module are the core of this
project. Each storage manager must implement a set of operations over
memory (see StorageManager.h for the exact interface). It also provides
`internalUpdate()` method, which signals that the storage manager should
perform some internal updates. For handling internal updates each storage
manager has a set of corresponding update managers, random and interactive
in particular.

### RA/SRA memory model

Each location holds a log of modifications sorted by timestamp. Each thread 
stores a so-called View: a collection of timestamps for each location. A thread
can only see messages with a timestamp greater or equal to the timestamp that
the thread observes. 

Modifications are represented by the class 
`wmm::storage::RA::Message`. Other than timestamp, location and stored value 
each message holds 1 or 2 views. 

* The base view - a view that must be acquired by the reading thread. 
This the view of the writing thread at the last release fence.
* An optional release view, which is attached to
release writes and acquired by acquire reads. It is equal to the view of 
the writing thread at the time of performing the write.

The difference between RA and SRA modes is as such: when choosing the timestamp
for a new write the SRA model picks a timestamp that is greater than the
greatest timestamp for the location at the time, but the RA model can pick any
timestamp that is greater than the timestamp the thread observes. If it happens
to be less than the greatest timestamp for the location, it is inserted into
the appropriate place in the log (the log is sorted by timestamp).

Further considerations are for atomic updates.
To perform an atomic update the thread first reads some message and then
performs a write depending on the value. To make sure that the update is atomic
the new message must come right after the read message in the log and no more
messages can be inserted between them. To achieve that there is a boolean field
in the Message class that marks whether the message was used by an atomic 
update.

### RA fences

There is an additional location in memory to facilitate
fences. Whenever there is a release fence, a release write is performed in that 
location. Also, the base view of the writing thread is updated to its current 
view. That way all subsequent writes will include the view of the thread
during the fence instruction.

### SC fences

Each thread acquires the current thread's current view.
That way in RA mode other threads can no longer perform writes before the
fencing thread's last write.

## Build & Execution

Use a compiler that supports C++20 standard. Though it should be ok to use any 
compiler that supports `std::format`.

Positional arguments:
1. Path to a program file
2. Memory model: one of `{sc, tso, pso, sra, ra}`
3. Execution mode: one of `{rand, interact}`
4. Log level: integer from `[0, 3]`. 
   * `0` - no log
   * `1` - errors only (no errors arise so it is the same as 0)
   * `2` - info, print trace of the execution and the state of the model in 
   the end
   * `3` - extra info, print both action log and model state after each step

Example command
```bash
examples/ra_fences.wmm ra rand 2
```