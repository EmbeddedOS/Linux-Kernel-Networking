# User Space Network Drivers

## Abstract

- The rise of user space packet processing frameworks like DPDK and netmap makes low-level code more accessible to developers and researchers.
- Previously, driver code was hidden in the kernel and rarely modified -- or even looked at -- by developers working at higher layers.

- The `ixy`, a user space network driver designed for simplicity and educational purposes to show that fast packet IO is not black magic but careful engineering.

- The `ixy` focuses on the bare essentials of user space packet processing: a packet forwarder including the whole NIC driver use less than 1000 lines of C code.

- The implementations of drivers for both the **Intel 82599 family** and for **virtual VirtIO NICs**.
  - The former allows us to reason about driver and framework performance on a stripped-down implementation to assess individual optimizations in isolation.
  - **VirtIO** support to ensures that everyone can run it in a virtual machine.

## Introduction

- **Low-level packet processing on top of traditional socket APIs is too slow** for modern requirements and was therefore often done in the kernel in the past.
- Writing kernel code is not only a relatively cumbersome process process with slow turn-around times, it also proved to be slow for specialized applications.

- Developers and researchers often treat DPDK as a black-box that magically increases speed.

- Abstractions hiding driver details from developers are an advantage:
  - they remove a burden from the developer. However, all abstractions are leaky, especially when performance critical code such as high-speed networking applications are involved.
  - We therefore believe it is crucial to have at least some insights into the inner workings of drivers when developing high-speed networking application.

- We present `ixy`, a user space packet framework that is architecturally similar to DPDK and Snabb. Both use full user space drivers, unlike netmap, PF_RING.

- We currently support the Intel ixgbe family of NICs and VirtIO NICs.

## Background and related work

- A multitude of packet IO frameworks have been built over the past years, each focusing on different aspects. They can be broadly categorized into two categories:
  - 1. Former categorize: Those relying on a driver running in the kernel.

    ```text
    User space
                  |Application|------>|mmap'ed |
                        ||            |Memory  |
                   |Magic lib|------->|        |
            Control API ||              ||
    |-------------------||--------------||------|
    Kernel space________||_________     ||
               |Magic_kernel_module|->|mmap'ed |
                     ___||___         |Memory  |
                    |_Driver_|------->|        |
                        ||
       //===============||=================\\
      _||__       _||__       _||__       _||__
     |_NIC_|     |_NIC_|     |_NIC_|     |_NIC_|
    ```

    - Example for Former categorize are `netmap`, `PF_RING`, `ZC`, `PFQ`, and `OpenOnload`.
      - They all use the default driver (somethings with small custom patches) and an additional kernel component that provides a fast interface based on memory mapping for the user space application.

    - Packet IO is still handled by the kernel here, but the driver is attached to the application directly instead of the kernel data-path.
    - This has advantage that integrating existing kernel components or forwarding packets to the default network stack is feasible with these frameworks.
  
    - By default, these application still provide an application with exclusive access to the NIC. Parts of the NIC can often still be controlled with standard tools like `ethtool` to configure packet filtering or queue sizes. However, hardware features are often poorly supported.

  - 2. Those that re-implement the whole driver in user space.

    ```text
    User space
                 |Application|------>|DMA     |
                        ||           |Memory  |
                  | mmap'ed |------->|        |
                  |   BAR0  |           ||
                        ||              ||
    |-------------------||--------------||------|
    Kernel space        ||              ||
                        ||              ||
                        ||              ||
       //===============||==============||==\\
     _||__       _||__       _||__         _||__
    |_NIC_|     |_NIC_|     |_NIC_|       |_NIC_|
    ```

    - DPDK, Snabb, and ixy implement the driver completely in user space. DPDK still uses a small kernel modules with some drivers, but it doesn't not contain driver logic and is only used during initialization.
    - Snabb and ixy require no kernel code at all.
    - A main advantage of the full user space approach is that the application has full control over the driver leading to a far better integration of the application with the driver and the hardware.
    - ixy is a full user space driver as we want to explore writing drivers and not interfacing with  existing drivers. Our architecture is based on ideas from both DPDK and Snabb.
      - The initialization and operation without loading a driver is inspired by Snabb, the API based on explicit memory management, batching, and driver abstraction is similar to DPDK.

## DESIGN

- Our design goals are:
  - 1. Simplicity. A forwarding application including a driver should be less than 1000 lines of C code.
  - 2. No dependencies. One self-contained project including the application and driver.
  - 3. Usability. Provide a simple-to-use interface for applications built on it.
  - 4. Speed. It should be reasonable fast without compromising simplicity, find the right trade off.

### Architecture

- `ixy` only features one abstraction level: it decouples the used driver from the user's application. Applications call into ixy to initialize a network device by its PCI address, ixy choses the appropriate driver automatically and returns a struct containing function pointers for driver-specific implementations. We currently expose packet reception, transmission, and device statistics to the application. Packet APIs are based on explicit allocation of buffers from specialized *memory pool* data structures.

- Applications include the driver directly, ensuring a quick turn-around time when modifying the driver. This means that the driver logic is only a single function call away from application logic, allowing the user to read the code from a top-down level without jumping between complex abstraction interfaces or even system calls.

### NIC Selection

- `ixy` is based on custom user space re-implementation of the Intel ixgbe driver and the VirtIO virtio-net driver cut down to their bare essentials.
- We chose ixgbe for ixy because Intel releases extensive data-sheets and the ixgbe NICs are commonly found in commodity servers. These NICs are also interesting because they expose a relatively low-level interface to the drivers.
  - NOTE: Other NICs like the newer Intel XL710 series or Mellanox ConnectX4/5 follow a more firmware-driven design: a lot of functionality is **hidden behind a black box firmware** running on the NIC and the driver merely communicates via a message interface with the firmware which does the hard work.
  - Our goal with ixy is understanding the full stack - a black-box firmware is counterproductive here and we have no plans to add support for such NICs.

- VirtIO was selected as second driver to ensure that everyone can run the code without hardware dependencies. A second interesting characteristic of VirtIO is that it is based on PCI instead of PCIe, requiring a different approach to implement the driver in user space.

### User Space Drivers in Linux

- There are two subsystems in Linux that enable user space drivers: `uio` and `vfio`, we support both.

- `uio` exposes all necessary interfaces to write full user space drivers via memory mapping files in the `sysfs` pseudo filesystem. These file-based APIs give us full access to the device without needing to write any kernel code. ixy unloads any kernel driver for the given PCI device to prevent conflicts, i.e., there is no driver loaded for the NIC while ixy is running.

- `vfio` offers more features: IOMMU and interrupts are only supported with `vfio`. However, these feature come at the cost of additional complexity: It requires binding the PICe device to the generic `vfio-pci` driver and it then exposes an API via `ioctl` system calls on special files.

- One needs to understand how a driver communicates with a device to understand how a driver can be written in user space. A driver can communicate via two ways with a PCIe device:
  - 1. The driver can initiate an access to the device's **Base Address Registers** (BARs).
  - 2. Or the device can initiate a **Direct Memory Access** (DMA) to access arbitrary main memory locations.

#### Accessing Device Registers

- MMIO maps a memory area to device IO, i.e, reading from or writing to this memory area receives receives/sends data from/to the device.
  - `uio` exposes all BARs in the `sysfs` pseudo filesystem, a privileged process can simply `mmap` them into its address space.
  - `vfio` provides an `ioctl` that returns memory mapped to this area. Devices expose their configuration registers via this interface where normal reads and writes can be used to access registers.
    - For example, ixgbe NICs expose all configuration, statistics, and debugging registers via the BAR0 address space. Our implementations of these mappings are in `pci_map_resource()` in [pci.c](https://github.com/emmericp/ixy/blob/b1cfa2240655f2644f7218abad3141236168f005/src/pci.c) and in `vfio_map_region()` in [libixy-vfio.c](https://github.com/emmericp/ixy/blob/b1cfa2240655f2644f7218abad3141236168f005/src/libixy-vfio.c).

- VirtIO (in the version we are implementing) is unfortunately based on PCI and not on PCIe and its BAR is an IO port resource that must be accessed with the archaic `IN` and `OUT` x86 instructions requiring IO privileges. Linux can grant processes the necessary privileges via `ioperm(2)`, DPDK uses this approach for their VirtIO driver.
