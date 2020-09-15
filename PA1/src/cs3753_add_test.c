#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

#define SYS_CS3753_ADD 333
int main(int agrc, char ** argv)
{

    int a = 2;
    int b = 4;

    int res;


    printf("Making system call with %d and %d\n", a, b);
    
    int result = syscall(SYS_CS3753_ADD, a, b, &res);

    printf("Result of the addition is: %d\n", res);
 
    return result;
}


