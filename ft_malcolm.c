#include <stdio.h>
#include <stdlib.h>

typedef struct malcolm {
    char *dest;
    char *src;
    char *dest_mac;
    char *src_mac;
} Malcolm;

int main(int argc, char *argv[])
{
    Malcolm malcolm;

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

    return 0;
}

