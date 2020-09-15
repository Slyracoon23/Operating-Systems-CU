#define _GNU_SOURCE 
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

#define SYS_HELLOWORLD 334
int main(int agrc, char ** argv)
{


    printf("Making system call helloworld\n");
    
    int result = syscall(SYS_HELLOWORLD);

    printf("Value of result: %d", result); 
    result = syscall(SYS_HELLOWORLD);
	
    printf("Value of result: %d", result); 
    return result;
}


