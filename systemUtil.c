#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <utmp.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <sys/utsname.h>

#include "systemUtil.h"



void first_part(int samples, int seconds)
{
    // This prints out the first part of the program which includes the given samples, seconds
    // and the amount of memory that has been utilized

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("Nbr of samples: %d every %d seconds\n", samples, seconds);
    printf("Memory usage: %ld KB\n", usage.ru_maxrss + usage.ru_minflt + usage.ru_ixrss + usage.ru_isrss + usage.ru_idrss + usage.ru_majflt + usage.ru_inblock + usage.ru_nswap + usage.ru_oublock + usage.ru_msgsnd + usage.ru_msgrcv);
    printf("----------------------------------------------------------------\n");
}

void cpuUsage(float *total_time, float *idle_time)
{

    // Opens the /proc/stat file and reads the information according to return the addition of all the
    // numbers minus the idle time
    long double user, nice, system, idle, IOwait, irq, softirq;
    FILE *fp = fopen("/proc/stat", "r");
    char string[255];
    fscanf(fp, "%s %Lf %Lf %Lf %Lf %Lf %Lf %Lf", string, &user, &nice, &system, &idle, &IOwait, &irq, &softirq);
    fclose(fp);
    float total = (float)user + (float)nice + (float)system + (float)IOwait + (float)irq + (float)idle + (float)softirq;
    *(total_time) = total;
    *(idle_time) = (float)(idle);
}

void core_usage(float old_total, float old_idle, int i, int graphics_check, int sequence_check, int pipe)
{
    // This function find the cpu usage by taking the 2 totals of the usage minus the idle
    // and then calculates the overall the change and putting into a percentage format
    float current_total = 0, current_idle = 0;
    cpu_stat cpu_info;
    if (i == 0)
    {
        cpuUsage(&current_total, &current_idle);
        cpu_info.total_time = current_total;
        cpu_info.total_idle = current_idle;
        cpu_info.total_cpu_usage = ((current_total - current_idle) / current_total) * ((float)100);
    }
    if (i > 0)
    {
        cpuUsage(&current_total, &current_idle);
        cpu_info.total_time = current_total;
        cpu_info.total_idle = current_idle;
        cpu_info.total_cpu_usage = (((current_total - current_idle) - (old_total - old_idle)) / (current_total - old_total)) * ((float)100);
    }
    ssize_t written = write(pipe, &cpu_info, sizeof(cpu_info));
    if (written == -1)
    {
        perror("Failed writing to pipe");
        return;
    }
    return;
}
void print_core_usage (float (*cpu_utilization)[3], int i, int graphics_check, int sequence_check)
{
      printf("Total CPU use = %.2f%%\n", cpu_utilization[i][2]); // sets the value of the cpu utilization to the array

    if (graphics_check == 1)
    {
        for (int j = 0; j <= i; j++)
        {
            if (sequence_check == 1)
            {
                if (j == i)
                {
                    printf("\t\t");
                    for (int q = 0; q < cpu_utilization[j][2] + 3; q++)
                    {
                        printf("|");
                    }
                    printf(" %f\n", cpu_utilization[j][2]);
                }
                else
                    printf("\n");
            }
            else
            {
                printf("\t\t");
                for (int q = 0; q < cpu_utilization[j][2] + 3; q++)
                {
                    printf("|");
                }
                printf(" %f\n", cpu_utilization[j][2]);
            }
        }
    }
    printf("----------------------------------------------------------------\n");

}

void system_info()
{
    // Prints out the system information

    struct utsname sysinfo;
    if (uname(&sysinfo) == 0)
    {
        printf("System name = %s\n", sysinfo.sysname);
        printf("Machine name = %s\n", sysinfo.nodename);
        printf("Version = %s\n", sysinfo.version);
        printf("Release = %s\n", sysinfo.release);
        printf("Architecture = %s\n", sysinfo.machine);
        fflush(stdout);
    }
    else
    {
        printf("Could not retrieve system information\n");
    }
    printf("----------------------------------------------------------------\n");
}

void session_users(int pipe)
{
    printf("### Sessions/users ###\n");
    // Finds the users currently on the session and prints them out
    char string[1024];
    struct utmp *user;
    setutent();
    while ((user = getutent()) != NULL)
    {
        if (user->ut_type == USER_PROCESS)
        {
            sprintf(string, "%s\t\t%s\t%s\n", user->ut_user, user->ut_line, user->ut_host);
            ssize_t written = write(pipe, string, strlen(string));
            if (written == -1)
            {
                perror("Failed writing to pipe");
                return;
            }
        }
    }
    endutent();
    return;
}

void print_session_users(int pipe)
{
    char pipe_buffer[1024]; // buffer to store pipe data
    ssize_t bytes_read = 0; // number of bytes read from the pipe
    while ((bytes_read = read(pipe, pipe_buffer, 1024)) > 0)
    {
        // loop while there is data available to be read from the pipe
        pipe_buffer[bytes_read] = '\0'; // make sure the buffer is null-terminated
        printf("%s", pipe_buffer);    // print the data read from the pipe
    }
    printf("----------------------------------------------------------------\n");
}

void memory_util(int samples, int seconds, float *phyical_memory, float *virtual_memory, int sequence_check, int graphics_check, int i, int pipe)
{
    // This function gathers information about the system using sysinfo, and prints out the values
    // in a given order of time frame where each samples is taken based on the seconds. Also stores the
    // given physical and virtual memory accordingly

    struct sysinfo info;
    if (sysinfo(&info) != 0)
    {
        perror("Error getting system information");
    }

    mem memory;
    memory.virtual_memory = ((info.totalswap - info.freeswap) + ((info.totalram - info.freeram) * info.mem_unit)) / ((float)1073741824);
    memory.physical_memory = ((info.totalram - info.freeram) * info.mem_unit) / ((float)1073741824);
    memory.totalram = info.totalram;
    memory.totalswap = info.totalswap;
    ssize_t written = write(pipe, &memory, sizeof(memory));
    if (written == -1)
    {
        perror("Failed writing to pipe");
        return;
    }
    return;
}

void print_memory_util(int samples, int seconds, float *phyical_memory, float *virtual_memory, int sequence_check, int graphics_check, int i, mem memory)
{
    printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");

    for (int j = 0; j < samples; j++)
    {
        if (sequence_check == 0)
        {
            if (j <= i)
            {
                printf("%.2f GB / %.2f GB   --  %.2f GB / %.2f GB", phyical_memory[j], memory.totalram / ((float)1073741824), virtual_memory[j], (memory.totalswap + memory.totalram) / ((float)(1073741824)));
                if (graphics_check == 0)
                    printf("\n");
                else
                {
                    printf("\t|");
                    if (j == 0)
                    {
                        printf("o %d (%f)\n", 0, virtual_memory[j]);
                    }
                    else
                    {
                        float difference = (virtual_memory[j] - virtual_memory[j - 1]);
                        for (int p = 0; p < abs((difference)*10); p++)
                        {
                            if (difference < 0)
                                printf(":");
                             else printf("#");
                        }
                        if (difference < 0)
                            printf("@ %f (%f)\n", difference, virtual_memory[j]);
                        else
                            printf("* %f (%f)\n", difference, virtual_memory[j]);
                    }
                }
            }
            else
                printf("\n");
        }
        else
        {
            if (j == i)
            {
                printf("%.2f GB / %.2f GB   --  %.2f GB / %.2f GB", phyical_memory[j], memory.totalram / ((float)1073741824), virtual_memory[j], (memory.totalswap + memory.totalram) / ((float)(1073741824)));
                if (graphics_check == 0)
                    printf("\n");
                if (graphics_check == 1)
                {
                    printf("\t|");
                    if (j == 0)
                    {
                        printf("o %d (%f)\n", 0, virtual_memory[j]);
                    }
                    else
                    {
                        float difference = (virtual_memory[j] - virtual_memory[j - 1]);
                        for (int p = 0; p < abs((difference)*10); p++)
                        {
                            if (difference < 0)
                                printf(":");
                            else
                                printf("#");
                        }
                        if (difference < 0)
                            printf("@ %f (%f)\n", difference, virtual_memory[j]);
                        else
                            printf("* %f (%f)\n", difference, virtual_memory[j]);
                    }
                }
            }
            printf("\n");
        }
    }
    printf("----------------------------------------------------------------\n");
}