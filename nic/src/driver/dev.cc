#include <util/log.hh>

#include <driver/virtio.hh>
#include "dev.hh"

std::shared_ptr<larva::Device> larva::Factory::init(const std::string &pci_bus_id)
{
    larva::PCI pci{pci_bus_id};
    if (pci._class_id != 0x0200)
    {
        fatal() << "PCI device " + pci_bus_id + " is not a NIC.";
    }

    if (pci._vendor_id == 0x1AF4 && pci._device_id == 0x1000)
    {
        debug() << "NIC device is virtio.";
        return std::make_shared<Virtio>();
    }
    else
    {
        /* TODO: Now we only support virtio. */
        return nullptr;
    }
}