#include <stdio.h>
#include <stdlib.h>



int main() {
    char msg[30];
    printf("MENSAJE CTM: \n");
    fgets(msg, sizeof(msg), stdin);
    printf("XDD %s\n", msg);

    return 0;
}