#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <sys/stat.h>

#include <util/log.hh>
#include <driver/io.hh>
#include "pci.hh"

larva::PCI::PCI(const std::string &pci_bus_id) : _pci_bus_id{pci_bus_id}
{
    IO io("/sys/bus/pci/devices/" + pci_bus_id + "/config", O_RDONLY);

    this->_vendor_id = io.read_io<uint16_t>(0);
    this->_device_id = io.read_io<uint16_t>(2);
    this->_class_id = io.read_io<uint16_t>(10);
}

bool larva::PCI::is_bind() const
{
    struct stat sb;
    std::string driver_path = "/sys/bus/pci/devices/" +
                              this->_pci_bus_id +
                              "/driver";

    return stat(driver_path.c_str(), &sb) == 0;
}

/**
 * @brief
 * - Unbind a device from driver. This done by writing the bus id of the
 *   device to unbind file. For example:
 *     $ echo `0000:00:08.0` > /sys/bus/pci/devices/0000:00:08.0/driver/unbind
 *
 * @return true if success, otherwise return false.
 */
bool larva::PCI::unbind()
{
    std::ofstream file{"/sys/bus/pci/devices/" + this->_pci_bus_id + "/driver/unbind",
                       std::ofstream::out};

    if (file)
    {
        file << this->_pci_bus_id;
        return true;
    }

    error() << "Failed to open unbind file.";
    return false;
}

bool larva::PCI::bind_to_kernel_virtio()
{
    std::ofstream file{"/sys/bus/pci/drivers/virtio-pci/bind",
                       std::ofstream::out};

    if (file)
    {
        file << this->_pci_bus_id;
        return true;
    }

    error() << "Failed to open bind file.";
    return false;
}
