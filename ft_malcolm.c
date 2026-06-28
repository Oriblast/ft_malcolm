#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netpacket/packet.h>

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
    uint16_t oper;      // request = 1
    uint8_t  sha[6];    // MAC source
    uint8_t  spa[4];    // IP source
    uint8_t  tha[6];    // MAC cible
    uint8_t  tpa[4];    // IP cible
} __attribute__((packed));


typedef struct malcolm {
    struct arp_header arp;
    struct ethernet_header eth;
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

int check_addr_mac(const char *mac, uint8_t hardwareAdrr[6])
{
    unsigned int b[6];

    if ( sscanf(mac,
                  "%02x:%02x:%02x:%02x:%02x:%02x",
                  &b[0], &b[1], &b[2],
                  &b[3], &b[4], &b[5]) != 6)
        return 0;
    for (int i = 0; i < 6; i++)
        hardwareAdrr[i] = b[i];
    return 1;
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

    malcolm.dest = argv[1];
    malcolm.src = argv[3];
    malcolm.dest_mac = argv[2];
    malcolm.src_mac = argv[4];

    printf("Destination: %s\n", malcolm.dest);
    printf("Source: %s\n", malcolm.src);
    printf("Destination MAC: %s\n", malcolm.dest_mac);
    printf("Source MAC: %s\n", malcolm.src_mac);

    if (!inet_pton(AF_INET, malcolm.src, &malcolm.arp.spa) || 
        !inet_pton(AF_INET, malcolm.dest, &malcolm.arp.tpa)) {
        fprintf(stderr, "Invalid IP address provided.\n");
        return 1;
    }
    if (!check_addr_mac(malcolm.dest_mac, malcolm.arp.tha) || !check_addr_mac(malcolm.src_mac, malcolm.arp.sha)) {
        fprintf(stderr, "Invalid MAC address provided.\n");
        return 1;
    }
    //memcpy(malcolm.arp.spa, inet_pton(AF_INET, malcolm.src, &malcolm.arp.spa), 4);
    // memcpy(malcolm.arp.tpa, inet_pton(AF_INET, malcolm.dest, &malcolm.arp.tpa), 4);
    malcolm.arp.htype = htons(1);
    malcolm.arp.ptype = htons(0x0800);
    malcolm.arp.hlen = 6;
    malcolm.arp.plen = 4; 
    malcolm.arp.oper = htons(1);
    
    return 0;
}

