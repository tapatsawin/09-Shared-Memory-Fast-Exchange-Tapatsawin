# System Programming Lab: Shared Memory Fast Exchange and Limits

## 1. Learning Objectives
By the end of this lab, you should be able to:
- exchange data quickly between processes with System V shared memory
- explain that shared memory exposes only the current value unless you build extra data structures
- observe fixed-capacity behavior from a bounded shared buffer
- detach and remove shared memory with `shmdt` and `shmctl`

## 2. Repository Layout
- `src/shared_memory_lab.c` - file you must edit (contains TODO sections)
- `include/shared_memory_lab.h` - shared struct and function prototypes (do not change)
- `scripts/` - visible checks and grading hooks
- `tests/` - short description of visible checks
- `samples/` - static sample strings used by the assignment

## 3. What You Need To Implement
Complete the three task functions in `src/shared_memory_lab.c`.

### Task A - `task_fast_exchange()`
Show the core advantage of shared memory.

### Task B - `task_latest_value_limit()`
Show one limitation: without extra structure, only latest value remains.

### Task C - `task_capacity_limit()`
Show fixed-buffer limitation with deterministic static input.
