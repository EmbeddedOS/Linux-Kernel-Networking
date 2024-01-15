#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include <driver/pci.h>

static void usage(int rc);

int main(int argc, char* argv[])
{
    char pci_bus_id[PCI_BUS_ID_MAX_LENGTH] = {0};
    char output_file[50] = {0};
    int opt = 0;
    while ((opt = getopt(argc, argv, "b:o:")) != -1) {
        switch (opt) {
        case 'b':
            strncpy(pci_bus_id, optarg, PCI_BUS_ID_MAX_LENGTH);
            break;
        case 'o':
            strncpy(output_file, optarg, 50);
            break;
        default:
            usage(EXIT_FAILURE);
        }
    }

    printf("PCI bus: %s\n", pci_bus_id);
    printf("out file: %s\n", output_file);

    exit(EXIT_SUCCESS);
}

static void usage(int rc)
{
    FILE *fp = rc ? stderr : stdout;
    fprintf(fp, "Usage: pcap [options] file...\n");
    fprintf(fp, "        -b <pci bus id>    PCI bus ID to capture packets.\n");
    fprintf(fp, "        -o <file>          Place the output into <file>.\n");
    exit(rc);
}
