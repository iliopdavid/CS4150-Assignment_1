# CS4150-Assignment_1
Assignment 1 for System Security Course

# System Call Tracer

A simple program to trace and log the system calls made by a target program during its execution. The tracer uses `ptrace` to monitor the child process and captures system call numbers, which are then saved to a file.

---

## Features

- Tracks and logs **unique system call numbers** invoked by a target program.
- Saves the list of system call numbers to a specified output file (`syscalls.txt`).
- Implements process tracing using the **`ptrace` system call**.
- Demonstrates fundamental concepts of process management and system call monitoring in Linux.

---

## Prerequisites

- A Linux-based system (required for `ptrace` and system call tracing).
- GCC or any C++ compiler that supports modern standards.
- You can install these by the following commands
```
sudo apt update
sudo apt install build-essential
sudo apt install gdb
```

---

## How It Works

1. The program forks a child process to run the target application.
2. The parent process attaches to the child process using `ptrace`.
3. Each time the child process invokes a system call, the parent intercepts it and logs the system call number.
4. Once the target program completes execution, all unique system call numbers are saved to `syscalls.txt`.

---

## Usage

### Compilation
Compile the program using a C++ compiler (beinf in the folder where the ccp file is):
```bash
g++ tracer -o tracer.cpp
```

### Execution
Run the program by using the following command:
```bash
./tracer /bin/ls
```
The program will:
- Execute `/bin/ls`.
- Log all unique system call numbers made by `/bin/ls`.
- Save the results to `/syscalls.txt`

The file contains the syscalls codes and not the commands(i.e. code=0, command=read). Here is the [table](https://filippo.io/linux-syscall-table/) with the code-commands matches.

### Output
The output file I have in the repository is the one after running the tracer for times.