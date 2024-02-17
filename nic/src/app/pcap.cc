#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include <pci.hh>
#include <driver/dev.hh>
#include <util/log.hh>

static void usage(int rc);

int main(int argc, char *argv[])
{
    std::string pci_bus_id{};
    std::string output_file{};
    int opt = 0;
    while ((opt = getopt(argc, argv, "hb:o:")) != -1)
    {
        switch (opt)
        {
        case 'b':
            pci_bus_id = std::string{optarg};
            break;
        case 'o':
            output_file = std::string{optarg};
            break;
        case 'h':
            usage(EXIT_SUCCESS);
            break;
        default:
            usage(EXIT_FAILURE);
        }
    }

    debug() << "PCI bus ID: " << pci_bus_id;
    std::shared_ptr<larva::Device> device = larva::Factory::init(pci_bus_id);
    exit(EXIT_SUCCESS);
}

static void usage(int rc)
{
    std::string s{"Usage: pcap [options] file...\n"
                  "        -b <pci bus id>    PCI bus ID to capture packets.\n"
                  "        -o <file>          Place the output into <file>.\n"};
    if (rc)
    {
        std::cerr << s;
    }
    else
    {
        std::cout << s;
    }

    exit(rc);
}
