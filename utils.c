//
// Created by novas on 16-6-21.
//

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

char* getEnv(char * name)
{
    char *temp=(char*)malloc(100);
    char *value=getenv(name);
    strcpy(temp,value);
    return temp;
}
char* getLogPath(int time,char *path)
{
    char *t=(char*)malloc(100);
    sprintf(t,"%d",time);
    strcat(t,".log");
    strcat(path,"/");
    strcat(path,t);
    printf("path=%s\n",path);
    return path;

}
char * getIp()
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                            sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            if(strcmp("127.0.0.1",host))
            {
                //printf("host=%s\n",host);
                return host;
            }
        }
    }
    freeifaddrs(ifaddr);
}