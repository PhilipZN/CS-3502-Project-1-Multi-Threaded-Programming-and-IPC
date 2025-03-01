
# CS 3502 Project 1 – Multi-Threaded Programming and IPC

## Overview
This project demonstrates key operating system concepts using C on Ubuntu. It is divided into two main parts:

1. **Project A: Multi-Threaded Programming Implementation** – A banking simulation implemented in four phases:
   - **Phase 1:** Basic Thread Operations  
     Create threads that perform simple operations concurrently. This phase shows basic thread creation and lifecycle management (without synchronization, so race conditions may occur).
   - **Phase 2:** Resource Protection  
     Use mutex locks to synchronize access to shared bank accounts. In this phase, multiple threads transfer \$50 from Account 0 to Account 1 safely.
   - **Phase 3:** Deadlock Creation  
     Demonstrate how deadlocks can occur naturally in multi-account transfers by allowing threads to lock accounts in inconsistent orders. The output logs show transfers until a natural deadlock halts progress; the parent process then detects the deadlock and terminates the child process.
   - **Phase 4:** Deadlock Resolution  
     Prevent and resolve deadlocks by enforcing a consistent lock ordering (always locking the lower-numbered account first) and using a timeout mechanism. Transfers that cannot acquire locks within the timeout are skipped, ensuring that the system continues operating.

2. **Project B: Inter-Process Communication (IPC)** – A custom program that processes the output of a command (e.g., `ls -l`) using UNIX pipes. The child process executes the command and sends its output through a pipe; the parent process reads and processes the output (numbering each line and summarizing statistics such as total lines and throughput).

Both parts include built-in unit tests and extensive logging to validate functionality, error handling, and performance.

## Directory Structure
```
Project1_OS/
├── README.md
├── src/
│   ├── multithreading_project.c   # Contains the multi-threading implementation (Phases 1–4 with tests)
│   └── ipc_project.c               # Implements the IPC solution (processing command output)
└── docs/
    └── report.tex                  # LaTeX source for the technical report

```

## Requirements
- **Operating System:** Ubuntu (running in VirtualBox is recommended)
- **Compiler:** GCC
- **Libraries:** POSIX Threads (pthreads)

## Building and Running

### Project A: Multi-Threading Implementation
1. Open a terminal and navigate to the repository's root.
2. Compile the multi-threading project:
   ```bash
   gcc -pthread -Wall -Wextra -o multithreading_project src/multithreading_project.c
   ```
3. Run the executable:
   ```bash
   ./multithreading_project
   ```
   The program will run all four phases sequentially, printing detailed logs for each phase:
   - **Phase 1:** Basic thread operations (showing potential race conditions).
   - **Phase 2:** Synchronized transfers using mutex locks.
   - **Phase 3:** Natural deadlock creation in multi-account transfers (with deadlock detection and termination).
   - **Phase 4:** Deadlock resolution with timeout and consistent lock ordering.

### Project B: Inter-Process Communication (IPC)
1. Compile the IPC project:
   ```bash
   gcc -pthread -Wall -Wextra -o ipc_project src/ipc_project.c
   ```
2. Run the executable:
   ```bash
   ./ipc_project [target_directory]
   ```
   If no target directory is specified, the program defaults to the current directory. The program will:
   - Execute `ls -l` on the specified directory.
   - Read the command output via a pipe.
   - Process the output by numbering each line and summarizing statistics such as total lines and data throughput.

## Testing
The project includes built-in unit tests based on the guidelines:
- **Multi-Threading Tests:**  
  Each phase of the multi-threading implementation logs detailed messages showing thread operations, lock acquisition and release, transfer details, deadlock occurrence, and resolution. Run the multi-threading executable to verify that:
  - Threads run concurrently (Phase 1).
  - Mutex locks correctly protect shared resources (Phase 2).
  - Deadlock occurs naturally in Phase 3 (with transfers printed until progress stops).
  - Deadlock resolution allows transfers to complete in Phase 4.
- **IPC Tests:**  
  The IPC program logs each line of output from the executed command and displays summary statistics (total lines, throughput). It also demonstrates robust error handling (e.g., detecting broken pipes or invalid directories).

## Report
A comprehensive LaTeX report is provided in the `docs` directory (`report.tex`). The report includes:
- An introduction explaining the project’s objectives and scope.
- A detailed description of the multi-threading implementation (Phases 1–4) and the IPC solution.
- Testing methodologies, sample outputs, and performance benchmarks.
- A discussion of challenges encountered and lessons learned.
- References and citations.

## Demonstration
A demonstration video is included in the final deliverables. The video shows:
- The Ubuntu VirtualBox setup.
- How to compile and run the multi-threading and IPC programs.
- Detailed explanation of output and design choices.

