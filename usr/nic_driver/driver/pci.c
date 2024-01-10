#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "pci.h"

/**
 * @brief
 * - Unbind a device from driver. This done by writing the bus id of the
 *   device to unbind file. For example:
 *     $ echo `0000:00:08.0` > /sys/bus/pci/devices/0000:00:08.0/driver/unbind
 * 
 * @param pci_addr - PCI address
 */
void pci_unbind(const char *pci_addr)
{
    char path[100] = {0};
    int fd = -1;
    ssize_t write_size = 0;
    snprintf(path, 100, "/sys/bus/pci/devices/%s/driver/unbind", pci_addr);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        // The device has been unbind.
        return;
    }

    write_size = write(fd, pci_addr, strlen(pci_addr));
    if (write_size != strlen(pci_addr)) {
        // Failed to unbind.
    }

    close(fd);
}