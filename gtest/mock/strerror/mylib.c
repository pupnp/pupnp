// Only usable with gcc. Compile for linking with
// gcc -pedantic-errors -Wall -c lib_a.c
// or to check the unmocked function:
// gcc -pedantic-errors -Wall -DEXECUTABLE -o lib_a.a lib_a.c
// and execute ./lib_a.a

#include <string.h>
#include <stdio.h>

int myStrerror()
{
    int error_number = 0;

    char *buffer = strerror(error_number);
    printf("Returned string = '%s'\n", buffer);

    return 0;
}

#if defined (EXECUTABLE)
int main(int argc, char **argv)
{
    return myStrerror();
}
#endif
