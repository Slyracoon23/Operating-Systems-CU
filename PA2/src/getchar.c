#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>

char buf[4];

int main(){

    //int input;

    scanf("%[^\t\n]s", &buf);

    printf("buffer: %s\n", buf);

    printf("buffer charachter: %s", buf[3]);

    return 0;

}
