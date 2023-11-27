# Weak Memory Models Mock

Supports SC, TSO, PSO and SRA memory models. Relaxed access in 
SRA is not supported yet. Has two execution modes: random and 
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