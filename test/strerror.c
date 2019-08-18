#include <stdio.h>
#include <string.h>
#include <errno.h>

int main()
{
    errno = 0;
    
    printf("%s\n", strerror(errno));
    
}
