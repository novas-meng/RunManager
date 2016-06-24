//
// Created by novas on 16/6/7.
//
#include <stdio.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include "utils.h"
#define maxProcessCount 3
typedef unsigned char Byte;
enum State{
    New,Running,Done,Error
};

struct dowork
{
    //时间戳，用来标示每一个任务
    int timestamp;
    //job完成情况
    enum State workState;
    //参数
    char buffer[1024];
    struct dowork *next;
};
typedef struct dowork* workList;
workList waitingList;
workList runningList;
//获取时间戳
double getTimeStamp(char *buffer)
{
    int index;
    int i=0;
    while (*(buffer+i)!=' ')
    {
        i++;
    }
    char time[i];
    for(index=0;index<i; index++)
    {
        time[index]=buffer[index];
    }
    return atof(time);
}
//读取socket传递过来的数据，数据格式为2_99_时间戳 算法名称 {m=2}{c=2}参数这样的形式
char* readSocket(int fd)
{
    char buffer[1];
    int i=0;
    int length=read(fd,buffer,sizeof(buffer));
    int W=buffer[0];
    int WW=W-48;
   // printf("%d\n",W);
    char ch[WW+2];
    read(fd,ch,sizeof(ch));
    char temp[WW];
    for(i=0;i<WW;i++)
    {
        temp[i]=ch[i+1];
    }
   // printf("temp=%s\n",temp);
     length=atoi(temp);
   // printf("length=%d\n",length);
    int nowLength=0;
    char res[length+1];
    char t[1024];
    while(nowLength!=length)
    {
       int readLength=read(fd,t,sizeof(t));
      //  printf("readlength=%d\n",readLength);
        for( i=0;i<readLength;i++)
        {
            res[nowLength+i]=t[i];
        }
        nowLength=nowLength+readLength;
    }
    res[length]='\0';
   // printf("res=%d\n",res);
    char *r=(char*)malloc(sizeof(res));
    strcpy(r,res);
   // printf("r=%s\n",r);
    return r;
}
struct dowork* getFirst(workList list)
{
    return list->next;
}
void delete(workList list,struct dowork *p)
{
    struct  dowork *q=list;
    while(q!=NULL)
    {
        if(q->next==p)
        {
            q->next=p->next;
            break;
        }
        q=q->next;
    }
}
struct dowork* outqueue(workList list)
{
    struct dowork *p=list->next;
    if(p==NULL)
    {
        return NULL;
    }
    list->next=p->next;
    return p;
}
//首先检查runningList中的个数，如果小于三，则从waitingList获取一个队头
void doRun(workList waitingList,workList runningList)
{
    printf("in dorun\n");
    int runningCount=0;
    struct dowork *p=runningList->next;
    while(p!=NULL)
    {
        //删除完成的工作
       if(p->workState==Done)
        {
            printf("完成的工作 %d\n",p);
            delete(runningList,p);
        }
        else if(p->workState==Running)
        {
            runningCount++;
        }
        p=p->next;
    }
    printf("runningcount=%d\n",runningCount);
    while (runningCount<maxProcessCount)
    {
        struct dowork* work=getFirst(waitingList);
        if(work==NULL)
        {
            printf("退出\n");
            break;
        }
        int pid;
        if(work->workState==New)
        {
            work=outqueue(waitingList);
            work->workState=Running;
            enqueue(runningList,work);
            int parentid=getpid();
            printf("执行,work=%s\n",work->buffer);
            if(pid=fork())
            {
                printf("增加\n");
                runningCount++;
            }
            else if(pid==0)
            {
               // system(work->buffer);
                char* path=getEnv("NOVAS_HOME");
                char* temp=(char*)malloc(100);
                strcpy(temp,path);
                strcat(path,"/jar/algocenter.jar ");
                char pre[]="hadoop jar ";
                strcat(pre,path);
                strcat(pre,work->buffer);
                printf("p=%s\n",pre);
                char *p=getLogPath(work->timestamp,temp);
                strcat(pre," 2>");
                strcat(pre,p);
                printf("做工作 %s\n",pre);
                system(pre);
                sleep(10);
               // kill(parentid,SIGUSR1);
                //发送完成信号
                union sigval mysigval;
                mysigval.sival_int=work->timestamp;

                printf("发送信号\n");
                sigqueue(parentid,SIGUSR1,mysigval);

                exit(1);
            }
        }
    }
    /*
  //  printf("in dorun");
    int pid;
    struct dowork* work;
    Label: work=getFirst(headList);
    printf("work=%d\n",work);
    if(work==NULL)
    {
        printf("empty\n");
    }
    else
    {
        if(work->workState==New)
        {
            work->workState=Running;
            int parentid=getpid();
            printf("执行结束,执行下一个,state=%s\n",work->buffer);
            if(pid=fork())
            {
                // exit(0);
            }
            else if(pid==0)
            {
                system(work->buffer);
                kill(parentid,SIGUSR1);
                exit(1);
                printf("发送信号\n");
            }
        }
        else if(work->workState==Running)
        {

        }
        else if(work->workState==Done)
        {
            printf("出队\n");
            outqueue(headList);
            goto Label;
        }
    }
     */
}
void handle(int signal,siginfo_t *info,void* mysact)
{
    if(signal==SIGUSR1)
    {
        printf("接收到信号\n");
        printf("time=%d\n",info->si_int);
        int timeStamp=info->si_int;
        struct dowork* p=runningList->next;
        while (p!=NULL)
        {
            if(p->timestamp==timeStamp)
            {
                p->workState=Done;
                break;
            }
            p=p->next;
        }
       // printf("fafd\n");
        umask(0);
        int logfd=open("/home/novas/work.log",O_CREAT|O_RDWR,777);
        char *time=(char*)malloc(100);
        sprintf(time,"%d",info->si_int);
        printf("logfd=%d\n",logfd);
        if(logfd!=-1)
        {
            write(logfd,time,strlen(time));
        }
        fsync(logfd);
        close(logfd);
        doRun(waitingList,runningList);
    }
}
void init(workList *list)
{
    *list=(struct dowork *)malloc(sizeof(struct dowork));
    printf("%d\n",list);
    (*list)->next=NULL;
}

void enqueue(workList list,struct dowork* work)
{
   // printf("===\n");

    struct dowork *p=list;

    //printf("===\n");
    while (p->next!=NULL)
    {
        p=p->next;
    }
  //  printf("===\n");

    p->next=work;
   // printf("===\n");

    work->next=NULL;
   // printf("===\n");

}
void sendAcceptOk(int fd)
{
    char *buffer;
    strcpy(buffer,"1_8_ACCEPTOK");
    write(fd,buffer,strlen(buffer));
    fsync(fd);
 //   close(fd);
}
//启动属性服务
int startConfManager(int port)
{
    char *ip=getIp();
    char portstr[10];
    sprintf(portstr,"%d",port);
    char *p=getEnv("NOVAS_HOME");
    strcat(p,"/jar/confmanager.jar ");
    strcat(p,ip);
    strcat(p," ");
    strcat(p,portstr);
    char cmd[100];
    strcpy(cmd,"java -jar ");
    strcat(cmd,p);
    printf("cmd=%s\n",cmd);
   // popen(cmd,"r");
}
int main()
{
    startConfManager(9081);
    init(&waitingList);
    init(&runningList);
   // signal(SIGUSR1,handle);
    //设置信号处理函数
    struct sigaction sa_user;
    sigemptyset(&sa_user.sa_mask);
    sa_user.sa_flags=SA_SIGINFO;
    sa_user.sa_sigaction=handle;
    sigaction(SIGUSR1,&sa_user,NULL);
    int fd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serverAddr;
    if(fd==-1)
    {
        printf("create roor");
    }
    memset(&serverAddr,0, sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=htonl(INADDR_ANY);
    serverAddr.sin_port=htons(9087);
    if(bind(fd,(struct sockaddr*)&serverAddr, sizeof(serverAddr))==-1)
    {
        printf("bind error\n");
    }
    if(listen(fd,10)==-1)
    {
        printf("listen error\n");
    }
    while (1)
    {
        printf("fd=%d\n",fd);
        int clientfd=-1;
      //  int clinetfd=accept(fd,(struct sockaddr*)NULL,NULL);
        for (;;)
        {
            if((clientfd=accept(fd,NULL, NULL)) < 0)
            {
                if (errno == EINTR)
                    continue;
                else
                {
                    printf("accept error");

                }
            }
            else
            {
                break;
            }
        }
        if(clientfd==-1)
        {
            printf("%s\n",strerror(errno));
            break;
        }
        char *buffer=readSocket(clientfd);
      //  printf("buffer=%d\n",buffer);
        printf("recvmsg=%s\n",buffer);
        struct dowork *work;
        work=(struct dowork*)malloc(sizeof(struct dowork));
        work->next=NULL;
        work->workState=New;
        strcpy(work->buffer,buffer);
        work->timestamp=getTimeStamp(work->buffer);
        printf("timestamp=%d",work->timestamp);
        printf("work=%d\n",work);
        enqueue(waitingList,work);
        sendAcceptOk(clientfd);
       // printf("%d",headList->next);
        doRun(waitingList,runningList);
        printf("done");
    }
}
