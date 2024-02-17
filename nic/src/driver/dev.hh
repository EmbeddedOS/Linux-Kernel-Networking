#pragma once
#include <memory>
#include <pci.hh>

namespace larva
{
    class Device
    {
    public:
        friend class Factory;
    };

    class Factory
    {
    public:
        static std::shared_ptr<Device> init(const std::string &pci_bus_id);
    };

};