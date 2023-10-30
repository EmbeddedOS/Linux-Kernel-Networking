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
  - we found it too cumbersome to initiate and use as it requires either parsing the PCIe configuration space or text files that, unlike their MMIO counterparts, cannot be `mapped`.
- These files can be opened and accessed via normal read and write calls that are then translated to the appropriate IO port commands by the kernel. We found this easier to use and understand but slower due to the required system call.
  - See `pci_open_resource()` in [pci.c](https://github.com/emmericp/ixy/blob/b1cfa2240655f2644f7218abad3141236168f005/src/pci.c) and `read/write_ioX()` in [device.h](https://github.com/emmericp/ixy/blob/b1cfa2240655f2644f7218abad3141236168f005/src/driver/device.h)

- A potential pitfall is that **the exact size of the read and writes are important**, e.g., accessing a single 32bit register with 2 16bit reads will typically fail and trying to read multiple small registers with one read might not be supported.
  - The exact semantics are up to the device, Intel's ixgbe NICs only expose 32 bit registers that support partial reads (except clear-on-read registers) but not partial writes.

- Virtio uses different register sizes and specifies that any access width should work in the mode we are using in practice only aligned and correctly sized accesses work reliably.

#### DMA in User space

- DMA is initiated by the PCIe device and allows it to read/write arbitrary physical ad
dresses. This is used to access packet data and to transfer the DMA descriptors (pointers to packet data) between driver and NIC.

- DMA needs to be explicitly enabled for a device via the PCI configuration space, our implementation is in `enable_dma()` in [pci.c](https://github.com/emmericp/ixy/blob/b1cfa2240655f2644f7218abad3141236168f005/src/pci.c) for `uio` and in `vfio_enable_dma()` in [libixy-vfio.c](https://github.com/emmericp/ixy/blob/b1cfa2240655f2644f7218abad3141236168f005/src/libixy-vfio.c) for `vfio`.

- DMA memory allocation differs significantly between `uio` and `vfio`.

##### uio DMA memory allocation

- Memory used for DMA transfer must stay resident in **physical memory**. `mlock()` can be used to disable swapping.
  - However this only guarantee that the physical address of the allocated memory stays the same. The linux page migration mechanism can change the physical address of any page allocated memory by the user space at any time, e.g, to implement transparent huge pages and NUMA optimizations.
  - Linux does not implement page migration of explicitly allocated huge pages (2MB or 1GB pages on x86). `ixy` therefore uses huge pages which also simplify allocating physically contiguous chunks of memory.
  - Huge pages allocated in user space are used by all investigated full user space drivers, but they are often passed off as a mere performance improvement despite being crucial for reliable allocation of DMA memory.

- The user space driver hence also needs to be able to translate its virtual addresses to physical addresses, this is possible via the `procfs` file `/proc/self/pagemap`, the translation logic is implemented in `virt_to_phys()` in [memory.c](https://github.com/emmericp/ixy/blob/b1cfa2240655f2644f7218abad3141236168f005/src/memory.c).

##### vfio DMA memory allocation

- The previous DMA memory allocation scheme is specific to a quirk in Linux on x86 and not portable.

- `vfio` features a portable way to allocate memory that internally calls `dma_alloc_coherent()` in the kernel like an in-kernel driver would.
- This system call abstracts all the messy details and is implemented in our driver in `vfio_map_dma()` in `libixy-vfio.c`.
- It requires an IOMMU and configures the necessary mapping to use virtual addresses for the device.

##### DMA and cache coherency

- Both of our implementations require a CPU architecture with cache-coherent DMA access. Older CPUs might not support this this and require explicit cache flushes to memory before DMA data can be read by the device.

- Modern CPUs do not have that problem. In fact, one of the main enabling technologies for high speed packet IO is that DMA accesses do not actually go to memory but to the CPU's cache on any recent CPU architecture.

#### Interrupts in User Space

- `vfio` features full support for interrupts, `vfio_setup_interrupt()` in [libixy_vfio.c](https://github.com/emmericp/ixy/blob/f0f2ce884ff6a14cd49d9b9aa1d3518c5a65c180/src/libixy-vfio.c) enables a specific interrupt for `vfio` and associates it with an eventfd file descriptor.

- `enable_msix_interrupt()` in [ixgbe.c](https://github.com/emmericp/ixy/blob/f0f2ce884ff6a14cd49d9b9aa1d3518c5a65c180/src/driver/ixgbe.c).

- Interrupts are mapped to a file descriptor on which the usual sys-calls like `epoll` are available to sleep until an interrupt occurs, see `vfio_epoll_wait()` in [libixy-vfio.c](https://github.com/emmericp/ixy/blob/f0f2ce884ff6a14cd49d9b9aa1d3518c5a65c180/src/libixy-vfio.c).

### Memory Management

- ixy builds on an API with explicit memory allocation similar to DPDK which is a very different approach from netmap that exposes a replica of the NIC's ring buffer to the application. Memory allocation for packets was cited as one of the main reasons why netmap is faster than traditional in kernel-processing.

- Hence, netmap lets the application handle memory allocation details. Many forwarding cases can then be implemented by simply swapping pointers in the rings. However, more complex scenarios where packets are not forwarded immediately to a NIC (e.g., because they are passed to a different core in a pipeline setting) do not map well to this API and require adding manual buffer management on top of this API.

- It is true the that memory allocation for packets is significant overhead in the Linux Kernel, we have measured a per-packet overhead of 100 cycles when forwarding packets with Open VSwitch on Linux for allocating and freeing packet memory (measured with `perf`).
  - The overhead is almost completely due to (re-)initialization of the kernel `sk_buff` struct: **a large data structure with a lot of metadata fields targeted at a general-purpose network stack**.

- Memory allocation in `ixy` (with minimum metadata required) only adds an overhead of 30 cycles/packet, a price that we are willing to pay for the gained simplicity in the user-facing API.

- ixy's API is the same as DPDK's API when it comes to sending and receiving packets and managing memory. It can best be explained by reading the example applications [ixy-fwd.c](https://github.com/emmericp/ixy/blob/b1cfa2240655f2644f7218abad3141236168f005/src/app/ixy-fwd.c) and [ixy-pktgen.c](https://github.com/emmericp/ixy/blob/b1cfa2240655f2644f7218abad3141236168f005/src/app/ixy-pktgen.c).

- the transmit-only example [ixy-pktgen.c](https://github.com/emmericp/ixy/blob/b1cfa2240655f2644f7218abad3141236168f005/src/app/ixy-pktgen.c) creates a *memory pool*, a fixed-size collection of fixed-size packet buffers and pre-fills them with packet data. It then allocates a batch of packets from this pool, adds a sequence number to the packet, and passes them to the transmit function.
  - The transmit function is asynchronous: it enqueues pointers to these packets, the NIC fetches and sends them later. Previously sent packets are freed asynchronously in the transmit function by checking the queue for sent packets and returning them to the pool.
  - This means that a packet buffer cannot be re-used immediately, the `ixy-pktgen` example look therefore quite different from a packet generator built on a classic socket API.

- the forward example [ixy-fwd.c](https://github.com/emmericp/ixy/blob/b1cfa2240655f2644f7218abad3141236168f005/src/app/ixy-fwd.c) can avoid explicit handling of memory pools in the application: the driver allocates a memory pool for each receive ring automatically allocates packets.


## IXGBE implementation

- IXGBE devices expose all configuration, statistics, and debugging registers via the BAR0 MMIO region.
- We use the `ixgbe_type.h` from Intel's driver as machine-readable version of data-sheet, it contains defines for all register names and offsets for bit fields.

### NIC Ring API

- **NICs expose multiple circular buffers called queues or rings to transfer packets**.
- The simplest setup uses only one receive and one transmit queue. Multiple transmit queues are merged on the NIC, incoming traffic is split according to filters or a hashing algorithm if multiple receive queues are configured.

- Both receive and transmit rings work in a similar way: **the driver programs a physical base address and the size of the ring**. It then fills the memory area with *DMA descriptors*, i.e, pointers to physical addresses where the packets data is stored with some metadata.
- Sending and receiving packets is done by passing ownership of the DMA descriptors between driver and hardware via a head and a tail pointer. The driver controls the tail, the hardware the head. Both pointers are stored in device registers accessible via MMIO.

- The initialization code is in [ixgbe.c](https://github.com/emmericp/ixy/blob/b1cfa2240655f2644f7218abad3141236168f005/src/driver/ixgbe.c#L173) starting from line 114 for receive queues and from line 173 for transmit queues.

#### Receiving packets

- The driver fills up the ring buffer with physical pointers to packet buffers in [start_rx_queue()](https://github.com/emmericp/ixy/blob/b1cfa2240655f2644f7218abad3141236168f005/src/driver/ixgbe.c#L64) on start up.

- Each time a packet is received, the corresponding buffer is returned to the application and we allocate a new packet buffer and store its physical address in the DMA descriptor and reset the ready flag.

- We also need a way to translate the physical addresses in the DMA descriptor found in the ring back to its virtual counterpart on packet reception. This is done by **keeping a second copy of the ring populated with virtual instead of physical addresses, this is then used as a lookup table for the translate**.

- DMA descriptors pointing into a memory pool, note that the packets in the memory are unordered as they can be free'd at different times:

  ```text
  Physical memory:

      ixgbe_adv_rx_desc.pkt_addr   
     //=======================\\
    ||     //==================||=====\\
    ||     ||     //===========||======||========\\
    ||     ||     ||           \/      \/         \/
  |16byte|16byte|16byte|...|2kilobyte|2kilobyte|2kilobyte|...      |
   \__________________/     \_____________________________________/
            ||                                ||
    Descriptor Ring                      Memory Pool
  ```

  - This figure illustrates the memory layout: the DMA descriptors in the ring to the left contain physical pointers to packet buffers stored in a separate location in a memory pool.
  - The packets buffers in the memory pool contain their physical address in a metadata field.

- Overview of a receive queue. The ring uses physical addresses and is shared with the NIC.

    ```text
                               |rx index|==========\\
                                ||                  \\//=======|RDT|
    |Virt. addr. of buffer 0|   ||                   ||
    |Virt. addr. of buffer 1|<==//           ________\/__
    |Virt. addr. of buffer 2|               /\\_0_||_1_//\
    |Virt. addr. of buffer 3|              /n / Rx Desc\ 2\
          Buffer Table                    |==|   Ring.  |==|
                                           \  \________/3 /
                                            \//___||___\\/
                                                /\
                                                ||
                                                \\============|RDH|
    ```

- This figure shows the RDH (Head) and RDT (Tail) registers controlling the ring buffer on the right side, and the local copy containing the virtual addresses to translate the physical addresses in the descriptors in the ring back for the application.

- `ixgbe_rx_batch()` in `ixgbe.c` implements the receive logic as described by *Sections 1.8.2 and 7.1 of the data-sheet*. **It operates on batches of packets to increase performance**.

- A native way to check if packets have been received is reading the head register from the NIC incurring a PCIe round trip. The hardware also sets a flag in the descriptor via DMA which is far cheaper to read as the DMA write is handled by the last-level cache on modern CPUs. This is effectively the difference between an LLC cache miss and hit for every received packet.

#### Transmitting Packets

- Transmitting packets follows the same concept and API as receiving them, but the function is more complicated because the interface between NIC and driver is asynchronous. Placing a packet into the ring does not immediately transfer it and blocking to wait for the transfer is infeasible.

- Hence, the `ixgbe_tx_batch()` function in `ixgbe.c` consists of two parts: freeing packets from previous calls that were sent out by the NIC followed by placing the current packets into the ring.
  - The first part is often called **cleaning and works** similar to receiving packets: the driver checks a flag that is set by the hardware after the packet associated with the descriptor is sent out. Sent packet buffers can the be free'd, making space in the ring.
  - Afterwards, the pointers of the packets to be sent are stored in the DMA descriptors and the tail pointer is updated accordingly.

- Checking for transmitted packets can be a bottleneck due to cache thrashing as both the device and driver access the same memory locations.
  - The 82599 hardware implements two method to combat this: marking transmitted packets in memory occurs either automatically in configurable batches on device side, this can also avoid unnecessary PCIe transfer. We tried different configurations (code in `init_tx()`) and found that the default from work Intel's work best. The NIC can also write its current position in the transmit ring back to memory periodically.

#### Batching

- Each successful transmit or receive operation involves an update to the NIC's tail pointer register (RDT or TDT for receive/transmit), a slow operation. This is one of reasons why batching is so important for performance. Both receive and transmit function are batched in ixy, updating the register only once per batch.

#### Offloading features

- ixy currently only enables CRC checksum offloading. Unfortunately, packet IO frameworks (e.g, netmap) are often restricted to this bare minimum of offloading features.
- DPDK is the exception here as it supports almost all offloading features offered by the hardware. However, its receive and transmit functions pay the price for
 these features in the form of complexity.

## VIRTIO Implementation

- All section numbers for the specification refer to version 1.0 of the VirtIO specification.

- VirtIO defines different types of operational modes for emulated network cards: **legacy**, **modern**, and **transitional devices**. qemu implements all three modes, the default being transitional devices supporting both the legacy and modern interface after feature negotiation. Supporting devices operating only in modern mode would be the simplest implementation in ixy because they work with MMIO.
  - Both legacy and transitional devices require support for PCI IO port resources making the device access different from the ixgbe driver. Modern-only devices are rare because they are relatively new.

- We chose to implement the legacy variant as VirtualBox only supports the legacy operation mode. VirtualBox is an important target as it is the only hypervisor supporting VirtIO that is available on all common OSs. Moreover, it is very well integrated with Vagrant allowing us to offer a self-contained setup to run ixy on any platform.
