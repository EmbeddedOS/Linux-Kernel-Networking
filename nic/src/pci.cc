#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <util/log.hh>
#include "pci.hh"

static inline uint16_t read_io16(int fd, size_t offset) {
	__asm__ volatile("" : : : "memory");
	uint16_t temp;
	if (pread(fd, &temp, sizeof(temp), offset) != sizeof(temp))
		error() << "pread io resource";
	return temp;
}

larva::pci::pci(const std::string &pci_bus_id): _pci_bus_id {pci_bus_id}
{
    std::ostringstream path {};
    path << "/sys/bus/pci/devices/" << this->_pci_bus_id << "/config";

    std::ifstream config_pci_file {};
    config_pci_file.open(path.str(), std::ios::binary);
    if (!config_pci_file) {
        fatal() << "Failed to open: " << path.str();
    }
    __asm__ volatile("" : : : "memory");

    //uint64_t config;
    uint16_t config;
    config_pci_file.read((char *)&config, 2);
    // std::vector<char> bytes(
    //      (std::istreambuf_iterator<char>(config_pci_file)),
    //      (std::istreambuf_iterator<char>()));

    // for (auto i : bytes) {
    //     std::cout << "0x" << std::hex << uint8_t(i) << ", ";
    // }
    // debug() << std::hex << bytes.at(2) << bytes.at(3);
    // debug() << bytes.size();
    config_pci_file.close();

    // int fd = open(path.str().c_str(), O_RDONLY);
    // uint16_t vendor_id = read_io16(fd, 0);
    //for (int i =0; i < 4; i++)
        debug() <<  std::hex << config;

}

bool larva::pci::is_bind() const
{
    return false;
}

void larva::pci::unbind()
{
    return;
}

void larva::pci::open_resource(const std::string &resource)
{
}

/**
 * @brief
 * - Unbind a device from driver. This done by writing the bus id of the
 *   device to unbind file. For example:
 *     $ echo `0000:00:08.0` > /sys/bus/pci/devices/0000:00:08.0/driver/unbind
 *
 * @param pci_addr - PCI structure
 */
// void pci_unbind(pci_t *pci)
// {
//     char path[PATH_MAX] = {0};
//     int fd = -1;
//     ssize_t write_size = 0;

//     snprintf(path,
//             PATH_MAX,
//             "/sys/bus/pci/devices/%s/driver/unbind",
//             pci->pci_bus_id);

//     fd = open(path, O_WRONLY);
//     if (fd < 0) {
//         // The device has been unbind.
//         return;
//     }

//     write_size = write(fd, pci->pci_bus_id, strlen(pci->pci_bus_id));
//     if (write_size != strlen(pci->pci_bus_id)) {
//         // Failed to unbind.
//     }

//     close(fd);
// }

// static int pci_open(pci_t *pci, const char *resource, int flags)
// {

// }
