#ifndef SYSTEMUTIL_H
#define SYSTEMUTIL_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <utmp.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <sys/utsname.h>

typedef struct mem_util{
    float physical_memory;
    float virtual_memory;
    float totalram;
    float totalswap;
} mem;

typedef struct cpu_stat{
    float total_time;
    float total_idle;
    float total_cpu_usage;
} cpu_stat;


void cpuUsage(float *total_time, float*idle_time);
void first_part(int samples, int seconds);

void session_users(int pipe);
void print_session_users(int pipe);

void memory_util(int samples, int seconds, float *phyical_memory, float *virtual_memory, int sequence_check, int graphics_check, int i, int pipe);
void print_memory_util(int samples, int seconds, float *phyical_memory, float *virtual_memory, int sequence_check, int graphics_check, int i, mem memory);

void system_info();

void core_usage(float old_total, float old_idle, int i, int graphics_check, int sequence_check, int pipe);
void print_core_usage(float (*cpu_utilization)[3], int i, int graphics_check, int sequence_check);
#endif /* SYSTEMUTIL_H */