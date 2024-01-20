# System Monitoring Program

## Overview

This C program provides a comprehensive system monitoring tool, collecting and displaying information on CPU usage, memory utilization, and active user sessions. The program utilizes various system calls and signals to retrieve and present real-time data.

## Dependencies

- Standard C Library (libc)
- Linux-specific libraries: `sys/resource.h`, `utmp.h`, `unistd.h`, `sys/sysinfo.h`, `string.h`, `sys/utsname.h`
- Custom header file: `systemUtil.h`

## Compilation

```bash
gcc systemMonitor.c systemUtil.c -o systemMonitor -lm
```

## Usage
The program supports the following command-line options:

--samples=<value>: Number of samples to collect (default: 10)
--tdelay=<value>: Time delay between samples in seconds (default: 1)
--sequential: Display output in sequential mode
--graphics or -g: Enable graphical representation of data
--user: Display information about active user sessions
--system: Display additional system information
Example usage:

```
bash
Copy code
./systemMonitor --samples=20 --tdelay=2 --sequential --graphics --user --system
```

## Features
CPU Usage: Displays total CPU usage over time, with an optional graphical representation.
Memory Utilization: Shows physical and virtual memory usage, along with graphical representation.
User Sessions: Lists users currently logged in along with their session details.
System Information: Provides basic information about the system.
Signals
The program responds to the following signals:

SIGINT (Ctrl-C): Gracefully terminates the program after user confirmation.
SIGTSTP (Ctrl-Z): Pauses the program, allowing the user to decide whether to continue or exit.

## Notes
The program uses forked processes to simultaneously collect CPU, memory, and user information.
Graphics mode displays a visual representation of data changes over time.



