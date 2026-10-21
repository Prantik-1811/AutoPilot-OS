# AutoPilot OS

AutoPilot OS is a Windows desktop automation project with an OS-independent C++20 planning model. The Stage A VM instructions are in [vm/README.md](vm/README.md).

## Build and test

Requirements: CMake 3.20+ and a C++20 compiler. CMake downloads the pinned GoogleTest release for the test target.

```powershell
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

## Current scope

Phase 1 defines typed `CreateDirectory` and `MoveFile` actions, allow-listed virtual state, and a deterministic simulated environment. Virtual paths use absolute `/`-separated names and case-sensitive comparison so the model does not inherit host filesystem semantics. The goal JSON Schema is in `schemas/goal.schema.json`; the C++ model does not parse JSON yet.

The simulator has no filesystem or Windows API access. It refuses out-of-scope paths and occupied destinations, and commits a simulated state transition only after checking its postconditions. Action costs and risks are neutral simulation placeholders; scoring is defined in Phase 2. Real adapters, persistence, planning, and UI are later phases.
