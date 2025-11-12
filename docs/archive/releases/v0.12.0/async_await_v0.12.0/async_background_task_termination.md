# Async Background Task Termination Behavior

**Version**: v0.12.0  
**Date**: 2025-11-08  
**Status**: Implemented

## Overview

Cb language implements a specific termination behavior for async background tasks: **when the main program exits, any remaining background tasks are immediately terminated without waiting for completion**.

## Behavior

### Current Implementation

```cb
async void task1() {
    println("Task1 start");
    sleep(500);
    println("Task1 end");  // This may not be printed
}

void main() {
    println("Main start");
    task1();  // Registers task in EventLoop
    println("Main end");  // Program exits immediately after this
}
```

**Output**:
```
Main start
Main end
```

**Note**: "Task1 start" and "Task1 end" are **NOT** printed because the main program exits before the background task can execute.

### Technical Details

1. **Task Registration**: When an `async` function is called without `await`, it is registered in the EventLoop as a background task
2. **Non-Blocking Execution**: The async function call returns immediately (returns 0)
3. **Program Termination**: When `main()` ends, the program terminates via `std::_Exit(0)`
4. **No Task Waiting**: The EventLoop destructor does not wait for pending tasks to complete

### Design Rationale

This behavior was specifically requested to avoid program hangs when background tasks are accidentally left running. Key benefits:

- **Predictable Termination**: Program always exits when main() completes
- **No Deadlocks**: Prevents situations where program waits indefinitely for stuck tasks
- **Clean Exit**: Fast program termination without cleanup overhead
- **Developer Control**: Developers must explicitly use `await` if task completion is required

## Comparison with Other Approaches

| Approach | Cb (Current) | Async/Await (Traditional) |
|----------|--------------|---------------------------|
| Main exits with pending tasks | Immediate termination | Wait or panic |
| Background task behavior | Terminates mid-execution | Completes or aborted explicitly |
| Developer requirement | Must use `await` for completion | Implicit waiting or explicit cancellation |

## Best Practices

### If You Need Task Completion

Use `await` to explicitly wait for tasks:

```cb
async void task1() {
    println("Task1 start");
    sleep(500);
    println("Task1 end");
}

void main() {
    println("Main start");
    
    // Option 1: await immediately (blocking)
    Future<void> f1 = task1();
    await f1;
    
    println("Main end");  // Task1 completes before this
}
```

**Output**:
```
Main start
Task1 start
Task1 end
Main end
```

### Fire-and-Forget Pattern

For truly fire-and-forget tasks where you don't care about completion:

```cb
async void logEvent(string message) {
    // Log to file or network
    // May not complete if main() exits
}

void main() {
    logEvent("Program started");
    // ... do work ...
    // Program may exit before log completes
}
```

**Note**: This pattern is suitable for:
- Best-effort logging
- Non-critical notifications
- Background cleanup tasks

**Warning**: Do NOT use for:
- Critical data writes
- Transaction commits
- Resource cleanup

## Future Enhancements

Potential features for future versions:

1. **Graceful Shutdown**: Option to wait for all pending tasks with timeout
2. **Task Priorities**: Allow critical tasks to block program exit
3. **Cancellation Tokens**: Explicit task cancellation mechanism
4. **Task Groups**: Wait for groups of related tasks

## Implementation Notes

### Code Location

- **EventLoop**: `src/backend/interpreter/event_loop/simple_event_loop.{h,cpp}`
- **Task Structure**: `src/backend/interpreter/event_loop/async_task.h`
- **Function Call Handler**: `src/backend/interpreter/evaluator/functions/call_impl.cpp` (line ~4860)
- **Program Termination**: `src/frontend/main.cpp` (line ~122, `std::_Exit(0)`)

### Key Implementation Details

```cpp
// In call_impl.cpp: async function handling
if (func->is_async) {
    // Create AsyncTask
    AsyncTask task;
    task.function_name = func->name;
    task.function_node = func;
    
    // Register in EventLoop (non-blocking)
    int task_id = interpreter_.get_event_loop().register_task(task);
    
    // Return immediately
    interpreter_.pop_scope();
    return 0;
}
```

```cpp
// In main.cpp: program termination
interpreter.process(root);

// Immediate exit without waiting for EventLoop
std::fflush(stdout);
std::fflush(stderr);
std::_Exit(0);  // Skips destructors, OS reclaims memory
```

## Related Features

- [Async/Await Design](async_await_design.md)
- [EventLoop Implementation](../architecture/event_loop.md)
- [Round-Robin Task Execution](async_round_robin.md)

## See Also

- Issue: "残っているバックグラウンドタスクがある場合は、それらが途中でもプログラムを終了することを望みます"
- Implementation PR: #XXX (to be created)
