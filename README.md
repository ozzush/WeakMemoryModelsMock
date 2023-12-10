# Weak Memory Models Mock

Supports SC, TSO, PSO and RA/SRA (with RA fences) memory models. Has two
execution modes: random and
interactive.

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