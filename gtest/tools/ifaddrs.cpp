#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>

class Ifaddr4
{
    struct sockaddr_in ifa_addr;      // network address
    struct sockaddr_in ifa_netmask;   // netmask
    struct sockaddr_in ifa_ifu;    // broadcast addr or point-to-point dest addr
    struct ifaddrs ifaddr;

public:
    Ifaddr4() {
//        // loopback interface
//        //-------------------
//        // set network address
//        ifa_addr.sin_family = AF_INET;
//        //ifa_addr.sin_port = htons(MYPORT);
//        inet_aton("127.0.0.1", &(ifa_addr.sin_addr));
//
//        // set netmask
//        ifa_netmask.sin_family = AF_INET;
//        //ifa_netmask.sin_port = htons(MYPORT);
//        inet_aton("255.0.0.0", &(ifa_netmask.sin_addr));
//
//        // set broadcast address or Point-to-point destination address
//        ifa_ifu.sin_family = 0;
//        //ifa_ifu.sin_port = htons(MYPORT);
//        inet_aton("0.0.0.0", &(ifa_ifu.sin_addr));
//
//        ifaddr.ifa_next = nullptr;  // pointer to next ifaddrs structure
//        ifaddr.ifa_name = (char*)"lo";
//        ifaddr.ifa_flags = 0;  // Flags from SIOCGIFFLAGS
//        ifaddr.ifa_addr = (struct sockaddr*)&ifa_addr;
//        ifaddr.ifa_netmask = (struct sockaddr*)&ifa_netmask;
//        ifaddr.ifa_broadaddr = (struct sockaddr*)&ifa_ifu;
//        ifaddr.ifa_data = nullptr;

        // testinterface
        //--------------
        // set network address
        ifa_addr.sin_family = AF_INET;
        //ifa_addr.sin_port = htons(MYPORT);
        inet_aton("192.168.168.168", &(ifa_addr.sin_addr));

        // set netmask
        ifa_netmask.sin_family = AF_INET;
        //ifa_netmask.sin_port = htons(MYPORT);
        inet_aton("255.255.255.0", &(ifa_netmask.sin_addr));

        // set broadcast address or Point-to-point destination address
        ifa_ifu.sin_family = 0;
        //ifa_ifu.sin_port = htons(MYPORT);
        inet_aton("192.168.168.255", &(ifa_ifu.sin_addr));

        ifaddr.ifa_next = nullptr;  // pointer to next ifaddrs structure
        ifaddr.ifa_name = (char*)"if0v4";
        ifaddr.ifa_flags = 0 | IFF_UP | IFF_MULTICAST; //Flags from SIOCGIFFLAGS
        ifaddr.ifa_addr = (struct sockaddr*)&ifa_addr;
        ifaddr.ifa_netmask = (struct sockaddr*)&ifa_netmask;
        ifaddr.ifa_broadaddr = (struct sockaddr*)&ifa_ifu;
        ifaddr.ifa_data = nullptr;
    }

    ifaddrs* get() {
        return &ifaddr;
    }
};
