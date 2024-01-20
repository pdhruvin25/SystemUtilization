#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <utmp.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <sys/utsname.h>
#include "systemUtil.h"
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

// Signal handler function for SIGINT
void signal_handler(int sig) {
    char ans;

    if (sig == SIGTSTP) {
        printf("\nCtrl-Z detected: ");
    } else if (sig == SIGINT) {
        printf("\nCtrl-C detected: ");
    }

    printf("Do you want to quit? (press 'y' if yes) ");

    // Wait for user input
    int ret = scanf(" %c", &ans);
    if (ret == EOF) {
        if (errno == EINTR) {
            printf("\nSignal detected during scanf, resuming...\n");
            return;
        } else {
            perror("scanf error");
            exit(EXIT_FAILURE);
        }
    }

    if (ans == 'y' || ans == 'Y') {
        exit(EXIT_SUCCESS);
    } else {
        printf("Resuming...\n");
    }
}

// Signal handler function for SIGTSTP
void handle_sigtstp(int sig)
{
    printf("\nReceived SIGTSTP signal. Cannot run in background while running interactively.\n");
}

int main(int argc, char *argv[])
{

    int graphics_check = 0, sequence_check = 0, user_check = 0, system_check = 0;
    int samples = 10, seconds = 1, samples_check = 0;



    // Reads all the flags
    for (int i = 0; i < argc; i++)
    {
        if (strstr(argv[i], "--samples=") != NULL)
        {
            char *str = strstr(argv[i], "=");
            samples = strtol(str + 1, NULL, 10);
        }
        if (strstr(argv[i], "--tdelay=") != NULL)
        {
            char *str = strstr(argv[i], "=");
            seconds = strtol(str + 1, NULL, 10);
        }

        if (strstr(argv[i], "--sequential") != NULL)
            sequence_check = 1;
        if (strstr(argv[i], "--graphics") != NULL || strstr(argv[i], "-g") != NULL)
            graphics_check = 1;
        if (strstr(argv[i], "--user") != NULL)
            user_check = 1;
        if (strstr(argv[i], "--system") != NULL)
            system_check = 1;
        for (int q = 0; argv[i][q]; q++)
        {
            if (argv[i][q] >= '0' && argv[i][q] <= '9' && samples_check == 0)
            {
                samples = strtol(argv[i], NULL, 10);
                samples_check = 1;
                break;
            }
            else if (argv[i][q] >= '0' && argv[i][q] <= '9' && samples_check == 1)
            {
                seconds = strtol(argv[i], NULL, 10);
                break;
            }
            else
                break;
        }
    }

    if (samples <= 0 || seconds <= 0)
    {
        printf("Number of samples or number of seconds cannot be negative\n");
        exit(EXIT_FAILURE);
    }

    // Calculates the cores of the machine in the beginning
    int cores = (int)sysconf(_SC_NPROCESSORS_ONLN);

    // The three arrays that are needed to store the values
    float virtual_memory[samples];
    float phyical_memory[samples];
    float cpu_utilization[samples][3];
    int memory_pipe[2], users_pipe[2], cpu_pipe[2];
    pid_t pid_memory, pid_users, pid_cpu;

    for (int i = 0; i < samples; i++)
    { // iterate through the number of samples

        struct sigaction act;
    act.sa_handler = signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    signal(SIGINT, signal_handler); // Register signal handler for SIGINT
    signal(SIGTSTP,handle_sigtstp); // Register signal handler for SIGTSTP


        if (pipe(memory_pipe) == -1 || pipe(users_pipe) == -1 || pipe(cpu_pipe) == -1)
        { // if pipe fails, an error message is printed and the program exits
            fprintf(stderr, "Pipe Failed");
            return 1;
        }

        // creates a child process for the memory information
        pid_memory = fork(); // forks a child process for memory information

        if (pid_memory < 0)
        {
            fprintf(stderr, "Fork Failed");
            return 2;
        }
        else if (pid_memory == 0)
        {

            signal(SIGINT, SIG_DFL);
            close(memory_pipe[0]);
            memory_util(samples, seconds, phyical_memory, virtual_memory, sequence_check, graphics_check, i, memory_pipe[1]);
            close(memory_pipe[1]);
            exit(0);
        }
        else
        {
            pid_users = fork();
            if (pid_users < 0)
            {
                fprintf(stderr, "Fork Failed");
                return 2;
            }
            else if (pid_users == 0)
            {
                signal(SIGINT, SIG_DFL);
                close(users_pipe[0]);
                session_users(users_pipe[1]);
                close(users_pipe[1]);
                exit(0);      // exit the child process
            }
            else
            {
                pid_cpu = fork();

                if (pid_cpu < 0)
                {
                    fprintf(stderr, "Fork Failed");
                    return 2;
                }
                else if (pid_cpu == 0)
                {
                    signal(SIGINT, SIG_DFL);
                    close(cpu_pipe[0]);
                    float old_total =0, old_idle = 0;
                    if(i > 0) old_total = cpu_utilization[i-1][0], old_idle = cpu_utilization[i-1][1];
                    core_usage(old_total, old_idle, i, graphics_check, sequence_check, cpu_pipe[1]);
                    close(cpu_pipe[1]);
                    exit(0);
                }
                else
                {
                    // main (parent) process
                    close(memory_pipe[1]); // close the write end of the pipe
                    close(users_pipe[1]);  // close the write end of the pipe
                    close(cpu_pipe[1]);    // close the write end of the pipe

                    mem memory;                                    // struct to store memory information
                    read(memory_pipe[0], &memory, sizeof(memory)); // read from the read end of the pipe
                    virtual_memory[i] = memory.virtual_memory;
                    phyical_memory[i] = memory.physical_memory;
                    close(memory_pipe[0]);                         // close the read end of the pipe

                    cpu_stat cpu_info;
                    read(cpu_pipe[0], &cpu_info, sizeof(cpu_info)); // reads the current cpu usage from the pipe
                    cpu_utilization[i][0] = cpu_info.total_time;
                    cpu_utilization[i][1] = cpu_info.total_idle;
                    cpu_utilization[i][2] = cpu_info.total_cpu_usage;
                    close(cpu_pipe[0]);                                       // close the read end of the pipe

                    waitpid(pid_memory, NULL, 0);
                    waitpid(pid_users, NULL, 0);
                    waitpid(pid_cpu, NULL, 0);

                    if (sequence_check == 1)
                    {
                        printf(">>>>>Iteration %d\n", i + 1);
                    }
                    else
                        printf("\033c");

                    // prints the number of samples, seconds and the memory usage
                    first_part(samples, seconds);

                    // The different posibilities
                    if (argc == 1)
                    {
                        //print_session_users(users_pipe[0]);

                        print_memory_util(samples, seconds, phyical_memory, virtual_memory, sequence_check, graphics_check, i, memory);
                        print_session_users(users_pipe[0]);
                        printf("Number of cores: %d\n", cores);
                        print_core_usage(cpu_utilization, i, graphics_check, sequence_check);
                    }

                    else if ((seconds != 1 || samples != 10) && sequence_check == 0 && graphics_check == 0 && user_check == 0 && system_check == 0)
                    {
                        print_memory_util(samples, seconds, phyical_memory, virtual_memory, sequence_check, graphics_check, i, memory);
                        print_session_users(users_pipe[0]);
                        printf("Number of cores: %d\n", cores);
                        print_core_usage(cpu_utilization, i, graphics_check, sequence_check);
                    }
                    else if (user_check == 1 || system_check == 1)
                    {
                        if (system_check == 1)
                        {
                        print_memory_util(samples, seconds, phyical_memory, virtual_memory, sequence_check, graphics_check, i, memory);
                        }
                        if (user_check == 1)
                        {
                            print_session_users(users_pipe[0]);
                        }
                        if (system_check == 1)
                        {
                            printf("Number of cores: %d\n", cores);
                            print_core_usage(cpu_utilization, i, graphics_check, sequence_check);
                        }
                    }
                    else if ((sequence_check == 1 || graphics_check == 1) && user_check == 0 && system_check == 0)
                    {
                        print_memory_util(samples, seconds, phyical_memory, virtual_memory, sequence_check, graphics_check, i, memory);
                        print_session_users(users_pipe[0]);
                        printf("Number of cores: %d\n", cores);
                        print_core_usage(cpu_utilization, i, graphics_check, sequence_check);
                    }
                    close(users_pipe[0]); // close the read end of the pipe
                    if (i + 1 != samples)
                        sleep(seconds); // this is to make sure the samples are seperated
                }
            }
        }
    }
    close(memory_pipe[0]); // close the read end of the pipe
    close(users_pipe[0]);  // close the read end of the pipe
    close(cpu_pipe[0]);    // close the read end of the pipe

    printf("### System Information ###\n");
    system_info();
    return 0;
}
