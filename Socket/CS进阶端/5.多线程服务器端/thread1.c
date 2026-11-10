#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

void *thread_main(void *arg)
{
    int cnt = *((int*)arg);
    for(int i = 0;i < cnt;i++)
    {
        sleep(1);
        puts("running thread");
    }
    char *msg = (char*)malloc(sizeof(char)*50);
    strcpy(msg,"Hello,I'm from thread~");
    return (void*)msg;
}

int main(int argc,char *argv[])
{
    pthread_t tid;
    int thread_param = 5;
    if(pthread_create(&tid,NULL,thread_main,(void*)&thread_param) != 0)
    {
        puts("pthread create error()");
        return -1;
    }
    void *thr_ret;
    if(pthread_join(tid,&thr_ret) != 0)
    {
        puts("pthread_join error()");
        return -1;
    }

    printf("message from pthread:%s\n",(char*)thr_ret);
    free(thr_ret);
    return 0;
}