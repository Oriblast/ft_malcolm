#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <netpacket/packet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025 // Maximum length of a hostname 
#endif

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 1
#endif
struct ethernet_header {
    uint8_t dest[6];   // MAC destination
    uint8_t src[6];    // MAC source
    uint16_t ethertype; // arp = 0x0806, ipv4 = 0x0800
} __attribute__((packed));

struct interphase {
    char *name;
    char *ip;
    char *mac;
};



struct arp_header {
    uint16_t htype;     // Ethernet = 1
    uint16_t ptype;     // IPv4 = 0x0800
    uint8_t  hlen;      // 6
    uint8_t  plen;      // 4
    uint16_t oper;      // request = 1 reponse egl 2 
    uint8_t  sha[6];    // MAC source
    uint8_t  spa[4];    // IP source
    uint8_t  tha[6];    // MAC cible
    uint8_t  tpa[4];    // IP cible
} __attribute__((packed));


typedef struct malcolm {
    struct arp_header arp;
    struct ethernet_header eth;
    struct interphase reseau;
    
    char *dest;
    char *src;
    char *dest_mac;
    char *src_mac;

} Malcolm;

Malcolm malcolm;
/*
int check_addr_mac(char *ip)
{
    char* token = strtok(ip, ":");
    int count = 0;

    while (token != NULL && count < 6)
    {
        token = strtok(NULL, ":");
        count++;
    }
    token = strtok(ip, ":");
    if (count == 6 )
    {
       
        return (1);
    }
    else
    {
        printf("Invalid MAC address: %s\n", ip);
        exit(1);
    }

}*/

static int hex_value(char c)
{
    if (c >= '0' && c <= '9')
        return (c - '0');
    if (c >= 'a' && c <= 'f')
        return (c - 'a' + 10);
    if (c >= 'A' && c <= 'F')
        return (c - 'A' + 10);
    return (-1);
}

int getInterfaceReseau(struct interphase *reseau)
{
    struct ifaddrs *ifaddr, *ifa;
    char ip[NI_MAXHOST];

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
                        ip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if(strcmp(ifa->ifa_name, "eth0") == 0) {
                reseau->name = strdup(ifa->ifa_name);
                reseau->ip = strdup(ip);
            }
            //printf("Interface: %s\tAddress: %s\n", ifa->ifa_name, ip);
        }
        if (ifa->ifa_addr->sa_family == AF_PACKET)
        {
            struct sockaddr_ll *s =
                (struct sockaddr_ll *)ifa->ifa_addr;
            if(strcmp(ifa->ifa_name, "eth0") == 0) {
                char mac[18];
                snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
                         s->sll_addr[0], s->sll_addr[1], s->sll_addr[2],
                         s->sll_addr[3], s->sll_addr[4], s->sll_addr[5]);
                reseau->mac = strdup(mac);
            }
            /*printf("Interface: %s\tMAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   ifa->ifa_name,
                   s->sll_addr[0],
                   s->sll_addr[1],
                   s->sll_addr[2],
                   s->sll_addr[3],
                   s->sll_addr[4],
                   s->sll_addr[5]);*/
        }

    }

    freeifaddrs(ifaddr);
    return 0;
}

int check_addr_mac(const char *mac, uint8_t hardwareAddr[6])
{
    int hi;
    int lo;
    int i;

    for (i = 0; i < 6; i++)
    {
        hi = hex_value(mac[i * 3]);
        lo = hex_value(mac[i * 3 + 1]);
        if (hi == -1 || lo == -1)
            return (0);
        hardwareAddr[i] = (hi << 4) | lo;
        if (i != 5 && mac[i * 3 + 2] != ':')
            return (0);
    }
    return (mac[17] == '\0');
}

int is_valid_ip(const char *ip)
{
    struct in_addr addr;

    return inet_pton(AF_INET, ip, &addr) == 1; // histoire de savoir si c'est valide
}

int main(int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <dest> <src> <dest_mac> <src_mac>\n", argv[0]);
        return 1;
    }

    malcolm.dest = argv[3];
    malcolm.src = argv[1];
    malcolm.dest_mac = argv[4];
    malcolm.src_mac = argv[2];

    printf("Destination: %s\n", malcolm.dest);
    printf("Source: %s\n", malcolm.src);
    printf("Destination MAC: %s\n", malcolm.dest_mac);
    printf("Source MAC: %s\n", malcolm.src_mac);

    if (!inet_pton(AF_INET, malcolm.src, &malcolm.arp.spa) || 
        !inet_pton(AF_INET, malcolm.dest, &malcolm.arp.tpa)) {
        fprintf(stderr, "Invalid IP address provided.\n");
        return 1;
    }
    if (!check_addr_mac(malcolm.dest_mac, malcolm.arp.tha) || 
            !check_addr_mac(malcolm.src_mac, malcolm.arp.sha)) {
        fprintf(stderr, "Invalid MAC address provided.\n");
        return 1;
    }
    //memcpy(malcolm.arp.spa, inet_pton(AF_INET, malcolm.src, &malcolm.arp.spa), 4);
    // memcpy(malcolm.arp.tpa, inet_pton(AF_INET, malcolm.dest, &malcolm.arp.tpa), 4);
    malcolm.arp.htype = htons(1);
    malcolm.arp.ptype = htons(0x0800);
    malcolm.arp.hlen = 6;
    malcolm.arp.plen = 4; 
    malcolm.arp.oper = htons(2);
    
    getInterfaceReseau(&malcolm.reseau);
    printf("Interface: %s\tAddress: %s\tMAC: %s\n", malcolm.reseau.name, malcolm.reseau.ip, malcolm.reseau.mac);

    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    memcpy(malcolm.eth.dest, malcolm.arp.tha, 6);
    memcpy(malcolm.eth.src, malcolm.arp.sha, 6);
    malcolm.eth.ethertype = htons(ETH_P_ARP);
    //suite

    free(malcolm.reseau.name);
    free(malcolm.reseau.ip);
    free(malcolm.reseau.mac);
    return 0;
}

