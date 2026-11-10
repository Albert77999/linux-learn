#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/times.h>
#include <sys/select.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    char buf[BUF_SIZE];
    fd_set reads, temp;
    FD_ZERO(&reads);
    FD_SET(0, &reads);
    struct timeval timeout;
    while (1)
    {
        temp = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        int retval = select(1, &temp, 0, 0, &timeout);
        if(retval == -1)
        {
            puts("select error!");
            break;
        }
        else if(retval == 0)
        {
            puts("time out");
            continue;
        }
        else
        {
            if(FD_ISSET(0,&temp))
            {
                int str_len = read(0,buf,BUF_SIZE - 1);
                if(str_len <= 0)
                {
                    puts("read error!");
                    break;
                }
                buf[str_len] = '\0';
                printf("message from console: %s\n",buf);
            }
        }
    }

    return 0;
}