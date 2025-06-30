# Fault

A C++ tool that writes random data into open file descriptors of targeted Linux processes, using the `pidfd_open` and `pidfd_getfd` system calls.
Useful for testing, fuzzing, or demonstrating concepts related to file descriptors and processes.

---

## Table of Contents

* [Features](#features)
* [Requirements](#requirements)
* [Compilation](#compilation)
* [Usage](#usage)
* [Detailed Options](#detailed-options)
* [Examples](#examples)
* [Important Notes](#important-notes)
* [Limitations and Security](#limitations-and-security)
* [License](#license)

---

## Features

* Lists open files for one or multiple specified PIDs, or for all processes on the system.
* Uses modern Linux system calls `pidfd_open` and `pidfd_getfd` (Linux kernel 5.6+).
* Sends random-sized data to each file descriptor.
* Supports repeated sending in a loop with configurable delay.
* Ignores SIGPIPE signals to prevent crashes on broken pipes.
* Debug mode with detailed logs.
* Built-in help with `-h` or `--help`.

---

## Requirements

* Linux kernel 5.6 or newer (for `pidfd_open` and `pidfd_getfd` support).
* C++17 compiler (e.g., g++ 7 or later).
* Sufficient privileges to access other processes’ file descriptors (usually root).

---

## Compilation

```bash
g++ -std=c++17 -O2 -o fault fault.cpp
```
Or 
```bash
make
```

---

## Usage

```bash
sudo ./fault <pid[,pid2,...]> [options]
sudo ./fault --lol [options]
```

* `<pid[,pid2,...]>`: comma-separated list of PIDs to target.
* `--lol`: target **all** processes on the system.

---

## Detailed Options

| Option         | Description                              | Default           |
| -------------- | ---------------------------------------- | ----------------- |
| `--min N`      | Minimum size of random data sent (bytes) | `1`               |
| `--max N`      | Maximum size of random data sent (bytes) | `1048576` (1 MiB) |
| `--limit N`    | Maximum number of iterations (loops)     | Unlimited         |
| `--delay S`    | Delay between each iteration (seconds)   | `0` (no delay)    |
| `--debug`      | Enable detailed debug output             | Disabled          |
| `-h`, `--help` | Show help message                        | N/A               |

---

## Examples

* Target PIDs `1234` and `5678`, send between 10 and 4096 bytes, repeat 10 times, with 0.5 seconds delay, debug enabled:

```bash
sudo ./fault 1234,5678 --min 10 --max 4096 --limit 10 --delay 0.5 --debug
```

* Target all processes on the system, unlimited loops, no delay, silent mode:

```bash
sudo ./fault --lol
```

* Show help:

```bash
./fault --help
```

---

## Important Notes

* **Privileges**: Accessing other processes’ file descriptors usually requires root or special capabilities.
* **Security**: This tool can write into file descriptors of other processes, potentially causing unexpected behavior, crashes, or corruption. Use with caution.
* **Compatibility**: Requires Linux kernel 5.6+ with `pidfd_open` and `pidfd_getfd` support.
* **SIGPIPE Signal**: Ignored globally to prevent the program from crashing on writing to closed pipes.
* Intended for experimental or testing use in controlled environments only.
