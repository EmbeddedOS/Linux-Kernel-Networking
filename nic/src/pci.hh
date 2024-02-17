#pragma once
/**
 * @file    pci.h
 * @author  Cong Nguyen (congnt264@gmail.com)
 * @brief
 *  - Accessing PCI device resources through sysfs:
 *      - sysfs, usually mounted at /sys, provides access to PCI resources on
 *        platforms that support it. For example, a given bus might look like
 *        this:
 *          /sys/devices/pci0000:17
 *          |-- 0000:17:00.0
 *          |   |-- class
 *          |   |-- config
 *          |   |-- device
 *          |   |-- enable
 *          |   |-- irq
 *          |   |-- local_cpus
 *          |   |-- remove
 *          |   |-- resource
 *          |   |-- resource0
 *          |   |-- resource1
 *          |   |-- resource2
 *          |   |-- revision
 *          |   |-- rom
 *          |   |-- subsystem_device
 *          |   |-- subsystem_vendor
 *          |   `-- vendor
 *          `-- ...
 *      - The topmost element describes the PCI domain and bus number. In this
 *        case, the domain number is `0000` and the bus number is 17 (both
 *        values are in hex). This bus contains a single function device in
 *        slot 0. The domain and bus numbers are reproduced for convenience.
 *        Under the device directory are several files, each with their own
 *        function.
 *          | file        | function                                           |
 *          |------------ |----------------------------------------------------|
 *          | class       | PCI class (ascii, ro)                              |
 *          | config      | PCI config space (binary, rw)                      |
 *          | device      | PCI device (ascii, ro)                             |
 *          | enable      | Whether the device is enabled (ascii, rw)          |
 *          | local_cpus  | Nearby CPU mask (cpumask, ro)                      |
 *          | remove      | Remove device from kernel's list (ascii, wo)       |
 *          | resource0.N | PCI resource N, if present (binary, mmap, rw)      |
 *          | resource0_wc..N_wc | PCI WC map resource N (binary, mmap)        |
 *          | revision    | PCI revision (ascii, ro)                           |
 *          | rom         | PCI ROM resource, if present (binary, ro)          |
 *          | subsystem_device | PCI subsystem device (ascii, ro)              |
 *          | subsystem_vendor | PCI subsystem vendor (ascii, ro)              |
 *          | vendor      | PCI vendor (ascii, ro)                             |
 *      - With:
 *          - ro - read only file.
 *          - rw - file is readable and writable.
 *          - wo - write only file.
 *          - mmap - file is mmapable.
 *          - ascii - file contains ascii text.
 *          - binary - file contains binary data.
 *          - cpumask - file contains a cpumask type.
 *      - The read only files are informational, writes to them will be ignored,
 *        with the exception of the `rom` file.
 *      - Writable files can be used to perform actions on the device (e.g.
 *        changing config space, detachable a device).
 *      - mmapable files are available via an `mmap` of the file at offset 0 and
 *        can be used to do actual device programming from userspace.
 *      - The 'enable' file provides a counter that indicates how many times the
 *        device has been enabled. If the 'enable' file currently returns '4',
 *        and a '1' is echoed into it, it will then return '5'. Echoing a '0'
 *        into it will decrease the count. Even when it returns to 0, though,
 *        some of the initialisation may not be reversed.
 *      - The 'remove' file is used to remove the PCI device, by writing a
 *        non-zero integer to the file. This does not involve any kind of
 *        hot-plug functionality, e.g. powering off the device. The device is
 *        removed from the kernel's list of PCI devices, the sysfs directory for
 *        it is removed, and the device will be removed from any drivers
 *        attached to it. Removal of PCI root buses is disallowed.
 */

#include <stdint.h>
#include <string>

namespace larva
{
    class PCI
    {
    public:
        std::string _pci_bus_id;
        uint16_t _vendor_id;
        uint16_t _device_id;
        uint16_t _class_id;

        explicit PCI(const std::string &pci_bus_id);
        PCI() = delete;
        ~PCI() = default;

        bool is_bind() const;
        bool unbind();
        bool bind_to_kernel_virtio();
    };
};
