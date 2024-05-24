#include <stdio.h>
#include <osbind.h>

void main(void)
{
    for (int i = 0; i < 16; ++i)
        printf("\033b/%2d\t\033b%cforeground\r\n", i, i + 32);

    printf("\r\npress the ANY key\r\n");
    Cconin();
}
