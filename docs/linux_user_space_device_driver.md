# Linux User space Device Drivers

## 1. Device Driver in user space article

- Traditionally, packet-processing or data-path applications in Linux have run in the kernel space due to the infrastructure provided by the Linux network stack. Frameworks such as netdevice drivers and net-filters have provided means for applications to directly hook into the packet-processing path within the kernel.

- However, a shift toward running data-path applications in the user space context is now occurring. The Linux user space provides several advantages for applications, including more robust and flexible process management, standardized system call interface, simpler resource management, large number of libraries for XML, and regular expression parsing, among others. It also makes applications more straightforward to debug by providing memory isolation and independent restart. At the same time, while kernel-space applications need to conform to GPL guidelines, user space applications are not bound by such restrictions.

- User-space data-path processing comes with is own overheads. Since the network drivers run in kernel context and use kernel-space memory for packet storage, there is an overhead of copying the packet data from user-space to kernel-space memory for vice versa. Also, user/kernel mode transitions usually impose a considerable performance overhead, thereby violates the low-latency and high-though-put requirements of data-path applications.

- In the rest of this article, we shall explore an alternative  approach to reduce these overheads for user-space data path applications.

### 1.1. Mapping memory to user space

- As an alternative to the traditional I/O model, the Linux kernel provides a user-space application with means to directly map the memory available to kernel to a user-space address range. **In the context of device drivers, this can provide user-space applications direct access to the device memory, which includes register configuration and I/O descriptors**. All accesses by the application to the assigned address range ends up directly accessing device memory.

- Several Linux system calls allow this kind of memory mapping, the simplest being the `mmap()` call. The `mmap()` call allows the user application to map a physical device address range one page at a time or a contiguous range of physical memory in multiples of page size.

- Other Linux system calls mapping memory include `splice()/vmsplice()`, which allows an arbitrary kernel buffer to be read or written to from user space, while `tee()` allows a copy between two kernel space buffers without access from user space.

- The task of mapping between the physical memory to the user-space memory is typically done using **Translation Look-aside Buffers** or **TLB** (**A translation look-aside buffer (TLB) is a memory cache that stores the recent translations of virtual memory to physical memory.**). The number of TBL entries in a given processor is typically limited and are thus used as a cache by the Linux kernel. The size of the memory region mapped by each entry is typically restricted to the minimum page size supported by processor, which is 4 kilobytes.

- Linux maps the kernel memory using a small set of TLB entries that are fixed during initialization time. For user-space applications however, the number of TLB entries are limited and each TLB miss can result a performance hit. To avoid such penalties, **Linux provides concepts of a Huge-TLB**, which allows user-space application to map pages large than the default minimum page size of 4KB. This mapping can be used not only for application data but text segment as well.

- Several efficient mechanisms have been developed in Linux to support zero-copy mechanisms between user space and kernel space based on memory mapping and other techniques. These can be used by the data-path applications while continuing to leverage the existing kernel-space network-driver implementation. However, **these mechanisms still consume the precious CPU cycles and per-packet-processing cost still remains moderately higher**.  Having a direct access to the hardware from the user space can eliminate the need for any mechanism to transfer packets back and forth between user space and kernel space, thus reducing the per-packet-processing cost.

### 1.2. UIO drivers

- Linux provides a standard UIO (User I/O) framework for developing user-space-based device drivers. The UIO framework defines a small kernel-space component that performs two key tasks:
  - 1. **Indicate device memory regions to user space**.
  - 2. **Register for device interrupts and provide interrupt indication to user space**.

- The kernel-space UIO component then exposes the device via a set of sysfs entries like `/dev/uioX`. The user-space component searches for these entries, reads the device address ranges and maps them to user space memory.

- The user-space component can perform all device-management tasks including I/O from the device. For interrupts however, it need to perform a blocking `read()` on the device entry, which results in the kernel component putting the user-space application to sleep and waking it up once an interrupt is received.

### 1.3. User-space network drivers

- The memory required by a network device driver can be of three types:
  - 1. **Configuration space**: This refers to the common configuration registers of the device.
  - 2. **I/O descriptor space**: This refers to the descriptors used by the device to access data from the device.
  - 3. **I/O data space**: this refers to the actual I/O data accessed from the device.

- Taking the case of a typical Ethernet device, the above can refer to the common device configuration (including MAC configuration), buffer-descriptor rings, and packet data buffers.

- In case of kernel-space network drivers, all three regions are mapped to kernel space, and any access to these from the user space is typically abstracted out via either `ioctl()` calls or `read()`/ `write()` calls, from where a copy of the data is provided to the user-space application.

- [Kernel space network driver](./resources/kernel_space_network_driver.png)
- [User space network driver](./resources/user_space_network_driver.png)

- User space network drivers, on the other hand, map all three regions directly to user space memory. This allows the user-space application to directly driver the buffer descriptor rings from user space. Data buffers can be managed and accessed directly by the application without overhead of a copy.

### 1.4. Constraints of user-space drivers

- Direct access to network devices brings its own set of complications for user-space applications, which were hidden by several layers of kernel stack and system calls:
  - 1. Sharing a single network device across multiple applications.
  - 2. Blocking access to network data.
  - 3. Lack of network stack services like TCP/IP.
  - 4. Memory management for packet buffers.
  - 5. Resource management across application restarts.
  - 6. Lack of standardized driver interface for applications.

## 2. Linux User Space Device Drivers

### 2.1. Device Driver Architectures

- Linux device drivers are typically designed as kernel driver running in kernel space.
- User space I/O is another alternative device driver architecture that has been supported by Linux Kernel since 2.6.24.

### 2.2. Legacy user driver method `/dev/mem`

- A character driver referred to as `/dev/mem` exists in the kernel that will map device memory into user space.
- With this driver user applications can access device memory.
- Memory access can be disabled in the kernel configuration as thi is big security hole (CONFIG_STRICT_DEVMEM).
  - Most production kernels for distributions are likely to have it turned off.
  - There is a distinction between memory (RAM) and devices which are memory mapped; devices are always allowed.
- Must be root user.
- A great tool for prototyping or maybe testing new hardware, but is not considered to be an acceptable production solution for a user space device driver.
- Since it can map any address into user space a buggy user space driver could crash the kernel.

### 2.3. Introduction to UIO

- The Linux kernel provides a framework for doing user space drivers called UIO.
- The framework is a character mode kernel driver (in drivers/uio) which runs as a layer under a user space driver.
- UIO helps to offload some of the work to develop a driver.
- The `U` in UIO is not for universal.
  - Devices well handled by kernel frameworks should ideally stay in the kernel (if you ask many kernel developers).
  - Networking is one of area where semiconductor vendors are doing user space I/O to get improved performance.

- UIO handles simple device drivers really well
  - Simple driver: device access and interrupt processing with no need to access kernel frameworks.

### 2.4. UIO Framework Features

- There are two distinct UIO device driver provided by Linux in `drivers/uio`.
- UIO Driver (`drivers/uio.c`)
  - For more advanced users as a minimal kernel space driver is required to setup the UIO framework.
  - This is the most universal and likely to handle all situations since the kernel space driver can be very custom.
  - The majority of work can be accomplished in the user space driver.
- UIO platform device driver (`drivers/uio_pdev_irqgen.c`)
  - This driver augments the UIO driver such that no kernel space driver is required:
    - It provides the required kernel space driver for uio.c
  - It works with device tree making it easy to use
    - The device tree node for the device needs to use `generic-uio` in it' compatible property.
  - Best starting point since no kernel space code is needed.

### 2.5. Kernel UIO API - Sys Filesystem

- The UIO driver in the kernel creates file attributes in the sys file system describing the UIO device.
- `/sys/class/uio` is the root directory for all the file attributes.
- A separate numbered directory structure is created under `/sys/class/uio` for each UIO device:
  - First UIO device: `/sys/class/uio/uio0`
  - `/sys/class/uio/uio0/name` contains the name of the device which correlates to the name in the `uio_info` structure.
  - `/sys/class/uio/uio0/maps` is a directory that has all the memory ranges for the device.
  - Each numbered map directory has attributes to describe the device memory including the address, name, offset and size.
    - `/sys/class/uio/uio0/maps/map0`

### 2.6. User Space Driver Flow

- 1. The kernel space UIO device driver(s) must be loaded before the user space driver is started.
- 2. The user space application is started and the UIO device file is opened (`/dev/uioX` where X is 0, 1, 2, etc.).
  - From user space, the UIO device, is a device node in the file system just like any other device.
- 3. The device memory address information is found from the relevant sysfs directory, only the size is needed.
- 4. The device memory is mapped into the process address space by calling `mmap()` function of the UIO driver.
- 5. The application accesses the device hardware to control the device.
- 6. The device memory is unmapped by calling `munmap()`.
- 7. The device file is closed.

```C
#define UIO_SIZE "/sys/class/uio/uio0/maps/map0/size"

int main(int argc, char **argv) {
    int uio_fd;
    unsigned int uio_size;
    FILE *size_fp;
    void *base_address;

    uio_fd = open("/dev/uio0", O_RDWR);
    size_fp = fopen(UIO_SIZE, O_RDONLY);
    fscanf(size_fp, "0x%08X", &uio_size);

    base_address = mmap(NULL, uio_size,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED, uio_fd, 0);
    // Access to the hardware can now occurs.

    munmap(base_address, uio_size);
}
```

### 2.7. Mapping Device Memory Details

- The character device driver framework of Linux provides the ability to map device memory into a user space process address space.
- A character driver implement the `mmap()` function which a user space application can call.
- The `mmap()` function creates a new mapping in the virtual address space of the calling process.
  - A virtual address, corresponding to the physical address specified is returned.
  - it can also be used to map a file into a memory space such that the contents of the file are accessed by memory reads and writes.
- Whenever the user space program reads or writes in the virtual address range it is accessing the device.
- This provides improved performance as no system calls are required.

### 2.8. User Space Application Interrupt Processing

- Interrupts are never handled directly in user space.
- The interrupt can be handled by the UIO kernel driver which then relays it on to user space via the UIO device file descriptor.
- The user space driver that wants to be notified when interrupts occur calls `select()` or `read()` on the UIO device file descriptor.
  - The read can be done as blocking or non-blocking mode.
- `read()` returns the number of events (interrupts).
- A thread could be used to handle interrupts.
- Alternatively a user provided kernel driver can handle the interrupt and then communicate data to the user space driver through other mechanisms like shared memory.
  - This may be necessary for devices which have very fast interrupts.

```C
int main(int argc, char **argv) {
    int uio_fd;
    int pending = 0;
    int re_enable = 1;

    uio_fd = open("/dev/uio0", O_RDWR);

    // Wait for an interrupt.
    read(uio_fd, (void *)&pending, sizeof(int));

    // Add device specific processing like acking the interrupt in the device here.

    // Re-enable the interrupt at the interrupt controller level.
    write(uio_fd, (void *)&re_enable, sizeof(int));
}
```

### 2.9. Advanced UIO With both User Space Application and Kernel Space Driver

- `struct uio_info`
  - name: device name
  - version: device driver version
  - irq: interrupt number
  - irq_flags: flags for request_irq()
  - handler: driver irq handler
  - mem[]: memory regions that can be mapped to user space.
    - addr: memory address.
    - memtype: type of memory region (physical, logical, virtual).

- The function `uio_register_device()` connects the driver to the UIO framework:
  - Requires a `struct uio_info` as an input.
  - Typically called from the `probe()` function of a platform device driver.
  - Creates device file `/dev/uio#` (# starting from 0) and all associated sysfs file attributes.
- The function `uio_unregister_device()` disconnects the driver from the UIO framework:
  - Typically called from the cleanup function of a platform device driver.
  - Deletes the device file `/dev/uio#`.

```C
void probe() {
    dev = devm_kzalloc(&pdev_>dev, sizeof(struct uio_timer_dev), GFP_KERNEL);
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    dev->regs = devm_ioremap_resource(&pdev->dev, res);
    irq = platform_get_irq(pdev, 0);
    dev->uio_info.name = "uio_timer";
    dev->uio_info.version = 1;
    dev->uio_info.priv = dev;

    // Add the memory region initialization for the UIO.
    dev->uio_info.mem[0].name = "registers";
    dev->uio_info.mem[0].addr = res->start;
    dev->uio_info.mem[0].size = resource_size(res);
    dev->uio_info.mem[0].memtype = UIO_MEM_PHYS;

    // Add the interrupt initialization for the UIO.
    dev->uio_info.irq = irq;
    dev->uio_info.handler = uio_irq_handler;
    uio_register_device(&pdev->dev, &dev->info);
}
```
