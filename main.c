#include "config.h"
#include <stdio.h>

int main(void)
{
    fprintf(stdout,"BUCKET_SIZE : %d\nPAGE_SIZE : %d\n", MAX_SIZE, PAGE_SIZE);
    return  0;
}
