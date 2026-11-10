#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

void *thread_main(void *arg)
{
    int start = ((int*)arg)[0];
    int end = ((int*)arg)[1];
    int* sum = (int*)malloc(sizeof(int)*4);
    while(start <= end)
    {
        *sum += start;
        start++;
    }
    return (void*)sum;
}

int main(int argc,char *argv[])
{
    pthread_t tid1,tid2;
    int range1[] = {1,5};
    int range2[] = {6,10};
    int sum = 0;

    pthread_create(&tid1,NULL,thread_main,(void*)range1);
    pthread_create(&tid2,NULL,thread_main,(void*)range2);

    void *ret_sum1;
    void *ret_sum2;
    pthread_join(tid1,&ret_sum1);
    pthread_join(tid2,&ret_sum2);
    sum = *((int*)ret_sum1) + *((int*)ret_sum2);
    printf("result : %i\n",sum);
    free(ret_sum1);
    free(ret_sum2);
    return 0;
}