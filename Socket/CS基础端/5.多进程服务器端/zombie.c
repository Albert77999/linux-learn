#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void read_childproc(int sig)
{
    int status;
    pid_t id = waitpid(-1, &status, WNOHANG);
    if (WIFEXITED(status))
    {
        printf("Removed proc id: %d \n", id);             // 子进程的 pid
        printf("Child send: %d \n", WEXITSTATUS(status)); // 子进程的返回值
    }
}

int main(int argc, char *argv[])
{
    struct sigaction act;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    act.sa_handler = read_childproc;
    sigaction(SIGCHLD, &act, 0);

    pid_t pid = fork();
    if (pid == 0)
    {
        printf("I am a child process\n");
        sleep(10);
        return 1;
    }
    else
    {
        printf("child process id: %d\n", pid);
        pid = fork();
        if (pid == 0)
        {
            puts("Hi! I'm child process");
            sleep(10);
            return 2;
        }
        else
        {
            printf("Child process id: %d\n", pid);
            for (int i = 0; i < 5; i++)
            {
                puts("wait...");
                sleep(5);
            }
        }
    }

    return 0;
}