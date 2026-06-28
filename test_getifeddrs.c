#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <stdio.h>

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 1
#endif

int main()
{
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return 1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        int family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            // IPv4 address
            getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                        host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            printf("Interface: %s\tAddress: %s\n", ifa->ifa_name, host);
        }
        if (ifa->ifa_addr->sa_family == AF_PACKET)
        {
            struct sockaddr_ll *s =
                (struct sockaddr_ll *)ifa->ifa_addr;

            printf("Interface: %s\tMAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   ifa->ifa_name,
                   s->sll_addr[0],
                   s->sll_addr[1],
                   s->sll_addr[2],
                   s->sll_addr[3],
                   s->sll_addr[4],
                   s->sll_addr[5]);
        }

    }

    freeifaddrs(ifaddr);
    return 0;
}