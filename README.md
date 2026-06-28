# OS-Nexus

From-scratch C++ simulators of the core resource managers inside an operating system: the **CPU scheduler**, the **virtual-memory manager**, the **I/O scheduler**, and a **two-pass linker**.

## Why this exists

An operating system is mostly a set of policies for sharing scarce resources — CPU time, physical memory, disk bandwidth — fairly and efficiently among competing processes. Those policies are easy to describe and surprisingly subtle to get right. OS-Nexus rebuilds each of them as a standalone simulator so the mechanics are explicit: how a scheduler decides who runs next, how paging turns a small amount of physical memory into a large virtual address space, how a disk scheduler orders requests to cut seek time, and how a linker stitches separately compiled modules into one runnable program. Building them from the ground up turns textbook concepts into working code whose behavior you can trace.

## Components

Each lives in its own directory with its source and build files.

### `Scheduler/` — CPU / process scheduler
Simulates process execution under multiple scheduling policies (e.g. first-come-first-served, shortest-remaining-time, round-robin, and priority/preemptive-priority), reading a process/event trace and reporting per-process and summary statistics (turnaround, wait time, CPU utilization) so the policies can be compared on the same workload.

### `Memory Management/` — virtual-memory manager
A demand-paging simulator that translates virtual addresses to physical frames, handles page faults, and reclaims frames using page-replacement algorithms, modeling the page table, frame table, and the cost of each memory reference.

### `IO Scheduler/` — disk I/O scheduler
Orders pending disk I/O requests under different disk-scheduling strategies (e.g. FIFO, shortest-seek-time-first, and elevator/LOOK-style algorithms) to reduce total seek movement, and reports the resulting service order and movement.

### `Linker/` — two-pass linker
Implements a classic two-pass linker: the first pass builds the symbol table and module base addresses; the second pass performs relocation and resolves external references across modules, with error and warning handling for unresolved or multiply-defined symbols.

> The simulators were exercised across 50+ processes to validate behavior under contention.

## Repository structure

```
Scheduler/           # CPU scheduling policies
Memory Management/   # demand paging + page replacement
IO Scheduler/        # disk request scheduling
Linker/              # two-pass linker
```

## Building and running

Each component is self-contained. From inside a component directory, compile its source (a `makefile` is provided where applicable), then run the produced executable against the input trace for that component, for example:

```bash
cd Scheduler
make
./scheduler <input-file> <options>
```

Refer to the source in each directory for that component's exact arguments and input format.

## Tech stack

C++ · Make

## Author

Mihir Prajapati — NYU, Operating Systems.
