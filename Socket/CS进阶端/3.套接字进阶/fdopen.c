#include <stdio.h>
#include <fcntl.h>

int main()
{
    int fd = open("data.dat",O_WRONLY | O_CREAT | O_TRUNC);
    if(fd == -1)
    {
        fputs("file open error",stderr);
        return -1;
    }
    FILE *fp = fdopen(fd,"w");
    fputs("NetWork Success!\n",fp);
    fprintf(fp,"转换成文件描述符:%d\n",fileno(fp));
    fclose(fp);
    //close(fileno(fp));
    return 0;
}

