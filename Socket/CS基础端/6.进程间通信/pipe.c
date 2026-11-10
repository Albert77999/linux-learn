#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

int main()
{
    int fds1[2], fds2[2];
    char message1[] = "Welcome to China!";
    char message2[] = "What's your problem?";
    char buffer[1024];
    pipe(fds1);
    pipe(fds2);

    pid_t pid = fork();
    if (pid == 0)
    {
        close(fds1[0]);
        close(fds2[1]);
        write(fds1[1], message1, sizeof(message1));
        read(fds2[0], buffer, 1024);
        printf("Message from proc: %s\n", buffer);
    }
    else
    {
        close(fds1[1]);
        close(fds2[0]);
        read(fds1[0], buffer, 1024);
        printf("Message from parent :%s\n", buffer);
        write(fds2[1], message2, sizeof(message2));
    }

    return 0;
}