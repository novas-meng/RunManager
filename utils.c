//
// Created by novas on 16-6-21.
//

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main()
{
    char *var="JAVA_HOME";
    char *value=getenv(var);
    if(value!=NULL)
    printf("%s",strerror(errno));
}