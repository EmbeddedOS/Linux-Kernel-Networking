#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <string.h>
#include <stdlib.h>

#include <util/log.hh>
#include "pci.hh"

larva::pci::pci(const std::string &pci_bus_id): _pci_bus_id {pci_bus_id}
{

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
