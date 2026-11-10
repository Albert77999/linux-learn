#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

char name[NAME_SIZE] = "[DEFAULT]";
char message[BUF_SIZE];

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(2);
}

void *send_msg(void *arg);
void *recv_msg(void *arg);

int main(int argc,char *argv[])
{
    struct sockaddr_in serv_addr;
    pthread_t snd_thread,rcv_thread;
    void *th_ret;
    if(argc != 4)
    {
        printf("Usage error()");
        exit(1);
    }
    sprintf(name,"[%s]",argv[3]);

    int clnt_sock = socket(AF_INET,SOCK_STREAM,0);
    if(clnt_sock == -1) error_handling("socket error()");
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(connect(clnt_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1)
        error_handling("connect error()");
    
    pthread_create(&snd_thread,NULL,send_msg,(void*)&clnt_sock);
    pthread_create(&rcv_thread,NULL,recv_msg,(void*)&clnt_sock);
    pthread_join(snd_thread,&th_ret);
    pthread_join(rcv_thread,&th_ret);
    close(clnt_sock);
    return 0;
}

void *send_msg(void *arg)
{
    int clnt_sock = (*(int*)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];
    while(1)
    {
        fgets(message,BUF_SIZE,stdin);
        if(!strcmp(message,"q\n") || !strcmp(message,"Q\n"))
        {
            close(clnt_sock);
            exit(0);
        }
        sprintf(name_msg,"%s:%s",name,message);
        write(clnt_sock,name_msg,strlen(name_msg));
    }
    return NULL;
}

void *recv_msg(void *arg)
{
    int clnt_sock = (*(int*)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];
    while(1)
    {
        int str_len = read(clnt_sock,name_msg,NAME_SIZE + BUF_SIZE - 1);
        if(str_len == -1)
            return (void*)-1;
        if(str_len == 0)
            break;
        name_msg[str_len] = 0;
        fputs(name_msg,stdout);
    }
    return NULL;
}