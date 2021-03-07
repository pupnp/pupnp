// very simple example to get only the name of a network interface
// Author: 2021-03-06 Ingo Höft <Ingo@Hoeft-online.de>

#include <stdio.h>
#include <ifaddrs.h>

int mygetifaddrs()
{
    struct ifaddrs* ifaddr = NULL;

    if (getifaddrs(&ifaddr) == -1) {
        return 1;
    }
    printf("mygetifaddrs: interface name = '%s'\n", ifaddr->ifa_name);
#if defined (EXECUTABLE)
    freeifaddrs(ifaddr);
#endif
    return 0;
}

#if defined (EXECUTABLE)
int main(int argc, char **argv)
{
    return mygetifaddrs();
}
#endif
