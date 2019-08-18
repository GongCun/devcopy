#include <stdio.h>
#include <errno.h>

int main()
{
    errno = 0;
    
    perror(NULL);
    perror("");
    
}
