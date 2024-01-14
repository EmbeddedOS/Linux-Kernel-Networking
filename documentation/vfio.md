# VFIO - "Virtual Function I/O"

- Many modern system now provide DMA and interrupt remapping facilities to help ensure I/O devices behave within the boundaries they've been allotted. The VFIO driver is an IOMMU/device agnostic framework for exposing direct device access to user-space, in a secure, IOMMU protected environment. In other words, this allows safe, non-privileged, user-space drivers.

- Why do we want that?
  - Virtual machines often make use of direct device access ("device assignment") when configured for the highest possible I/O performance. From a device and host perspective, this simply turns the VM into a user-space driver, with the benefits of significantly reduced latency bandwidth, and direct use of bare-metal device drivers.

- Some applications, particularly in the high performance computing field, also benefit from-overhead, direct device access from user-space. Example include network adapters (often non-TCP/IP based) and compute accelerators. Prior to VFIO, these drivers has to either go through the full development cycle to become proper up-stream driver, be maintained out of tree, *or make use of the UIO framework, which has no notion of IOMMU protection, limited interrupt support, and requires root privileges to access things like PCI configuration space*.

- The VFIO driver framework, intends to unify these, replacing both the KVM PCI specific device assignment code as well as provide a more secure, more feature-ful user-space driver environment than UIO.

## IOMMU

- IOMMU standards **input-output memory management unit** is a MMU connecting DMA-capable I/O bus to the main memory. Like a traditional MMU, which translates CPU-visible virtual addresses to physical addresses, **the IOMMU maps device-visible virtual addresses to physical address**.

- Advantages: The advantages of having an IOMMU, compared to direct physical address of the memory (DMA), include:
  - 1. Large regions of memory can be allocated without the need to be contiguous in physical memory - The IOMMU maps contiguous virtual addresses to the underlying fragmented physical addresses.
  - 2. Devices that do not support memory addresses long enough to address the entire physical memory can still address the entire memory through the IOMMU, avoiding overheads associated with copying buffers to and from the peripheral's addressable memory space.
  - 3. Memory is protected from malicious devices that are attempting DMA attacks. because a device cannot read or write to memory that has not been explicitly allocated (mapped) for it.

- Disadvantages:
  - Some degradation of performance from translation and management overhead (e.g., page walks).
  - Consumption of physical memory for the address I/O page tables.
  - In order to decrease the page table size the granularity of many IOMMUs is equal to the memory paging (often 4096 bytes), and hence each small buffer that needs protection against DMA attack has to be page aligned and zeroed before making visible to the device.

- Virtualization:
  - When an OS is running inside a virtual machine, including systems that use para-virtualization, such as KVM, it does not usually know the host-physical addresses of memory that it accesses. This makes providing direct access to the computer hardware difficult, because if the guest OS tried to instruct the hardware to perform a direct memory access (DMA) using guest-physical addresses, it would likely corrupt the memory, as the hardware does not know about the mapping between the guest-physical and host-physical addresses for the given virtual machine. The corruption can be avoided if the hypervisor or host OS intervenes in the I/O operation to apply the translations. However, this approach incurs a delay in the I/O operation.
  - An IOMMU solves this problem by re-mapping the addresses accessed by the hardware according to the same (or a compatible) translation table that is used to map guest-physical address to host-physical addresses.

## Groups, Device, and IOMMUs

- **Devices are the main target of any I/O driver. Devices typically create a programming interface made up of I/O access, interrupts, and DMA.**

- Without going into the details of each of these, DMA is by far the most critical aspect for maintaining a secure environment as allowing a device read-write access to system memory imposes the greatest risk to the overall system integrity.

- To help mitigate this risk, many modern IOMMUs now incorporate isolation properties into what was, in many cases, an interface only meant for translation (ie. solving the addressing problems of devices with limited address spaces). With this devices can now be isolated from each other and from arbitrary memory access, this allowing things like secure direct assignment of devices into VMs.

### Group

- This isolation is not always at the granularity of a single device though. Even when an IOMMU is capable of this, properties of devices, interconnects, and IOMMU topologies can each reduce this isolation. For instance, an individual device may be part of a larger multi- function enclosure.

- Therefore, while for the most part an IOMMU may have device level granularity, any system is susceptible to reduced granularity. **The IOMMU API therefore supports a notion of IOMMU groups**.
  - **A group is a set of devices which is isolable from all other devices in the system**. Groups are therefore the unit of ownership used by VFIO.

- VFIO makes use of a container class, which may hold one or more groups. A container is created by simply opening the `/dev/vfio/vfio` character device.

- On its own, the container provides little functionality, with all but a couple version and extension query interfaces locked away. The user needs to add a group into the container for the next level of functionality.
  - 1. To do this, the user first needs to identify the group associated with the desired device.
  - 2. By unbinding the device from the host driver and binding it to a VFIO driver, a new VFIO group will appear for the group as `/dev/vfio/$GROUP`, where `$GROUP` is the IOMMU group number of which the device is a member.
  - If the IOMMU group contains multiple devices, each will need to be bound to a VFIO driver before operations on the VFIO group are allowed.
  - TBD - interface for disabling driver probing/locking a device.
  - Once the group is ready, it may be added to the container by opening the VFIO group character device (`/dev/vfio/$GROUP`) and using the VFIO_GROUP_SET_CONTAINER ioctl, passing the file descriptor of the previously opened container file.

- With a group (or groups) attached to a container, the remaining ioctls become available, enabling access to the VFIO IOMMU interfaces. Additionally, it now becomes possible to get file descriptors for each device within a group using an ioctl on the VFIO group file descriptor.

- The VFIO device API includes ioctls for describing the device, the I/O regions and their read/write/mmap offsets on the device descriptor, as well as mechanisms for describing and registering interrupt notifications.

## VFIO Usage Example

- Assume user wants to access PCI device 0000:06:0d.0:

```bash
readlink /sys/bus/pci/devices/0000:06:0d.0/iommu_group
```

- Output:

```text
../../../../kernel/iommu_groups/26
```

- This means device is therefore in IOMMU group 26. This device is on the pci bus, therefore the user will make use of vfio-pci to manage the group:

```bash
modprobe vfio-pci
```

- binding this device to the vfio-pci driver creates the VFIO group character devices for his group:

```bash
lspci -n -s 0000:06:0d.0
# Output: 06:0d.0 0401: 1102:0002 (rev 08)

# unbind.
echo 0000:06:0d.0 > /sys/bus/pci/devices/0000:06:0d.0/driver/unbind

# binding device to vfio-pci driver.
echo 1102 0002 > /sys/bus/pci/drivers/vfio-pci/new_id
```

- Now we need to look at what other devices are in the group to free it for use by VFIO:

```bash
ls -l /sys/bus/pci/devices/0000:06:0d.0/iommu_group/devices
```

- Output:

```text
total 0
lrwxrwxrwx. 1 root root 0 Apr 23 16:13 0000:00:1e.0 ->
        ../../../../devices/pci0000:00/0000:00:1e.0
lrwxrwxrwx. 1 root root 0 Apr 23 16:13 0000:06:0d.0 ->
        ../../../../devices/pci0000:00/0000:00:1e.0/0000:06:0d.0
lrwxrwxrwx. 1 root root 0 Apr 23 16:13 0000:06:0d.1 ->
        ../../../../devices/pci0000:00/0000:00:1e.0/0000:06:0d.1
```

- The final step is to provide the user with access to the group if unprivileged operation is desired (note that `/dev/vfio/vfio` provides no capabilities on its own and is therefore expected to be set to mode 0666 by the system):

```bash
chown user:user /dev/vfio/26
```

- The user now has full access to all the devices and the iommu for this group and can access them as follows:

```C
int container, group, device, i;
struct vfio_group_status group_status =
                                { .argsz = sizeof(group_status) };
struct vfio_iommu_type1_info iommu_info = { .argsz = sizeof(iommu_info) };
struct vfio_iommu_type1_dma_map dma_map = { .argsz = sizeof(dma_map) };
struct vfio_device_info device_info = { .argsz = sizeof(device_info) };

/* Create a new container */
container = open("/dev/vfio/vfio", O_RDWR);

if (ioctl(container, VFIO_GET_API_VERSION) != VFIO_API_VERSION)
        /* Unknown API version */

if (!ioctl(container, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU))
        /* Doesn't support the IOMMU driver we want. */

/* Open the group */
group = open("/dev/vfio/26", O_RDWR);

/* Test the group is viable and available */
ioctl(group, VFIO_GROUP_GET_STATUS, &group_status);

if (!(group_status.flags & VFIO_GROUP_FLAGS_VIABLE))
        /* Group is not viable (ie, not all devices bound for vfio) */

/* Add the group to the container */
ioctl(group, VFIO_GROUP_SET_CONTAINER, &container);

/* Enable the IOMMU model we want */
ioctl(container, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU);

/* Get addition IOMMU info */
ioctl(container, VFIO_IOMMU_GET_INFO, &iommu_info);

/* Allocate some space and setup a DMA mapping */
dma_map.vaddr = mmap(0, 1024 * 1024, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
dma_map.size = 1024 * 1024;
dma_map.iova = 0; /* 1MB starting at 0x0 from device view */
dma_map.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE;

ioctl(container, VFIO_IOMMU_MAP_DMA, &dma_map);

/* Get a file descriptor for the device */
device = ioctl(group, VFIO_GROUP_GET_DEVICE_FD, "0000:06:0d.0");

/* Test and setup the device */
ioctl(device, VFIO_DEVICE_GET_INFO, &device_info);

for (i = 0; i < device_info.num_regions; i++) {
        struct vfio_region_info reg = { .argsz = sizeof(reg) };

        reg.index = i;

        ioctl(device, VFIO_DEVICE_GET_REGION_INFO, &reg);

        /* Setup mappings... read/write offsets, mmaps
         * For PCI devices, config space is a region */
}

for (i = 0; i < device_info.num_irqs; i++) {
        struct vfio_irq_info irq = { .argsz = sizeof(irq) };

        irq.index = i;

        ioctl(device, VFIO_DEVICE_GET_IRQ_INFO, &irq);

        /* Setup IRQs... eventfds, VFIO_DEVICE_SET_IRQS */
}

/* Gratuitous device reset and go... */
ioctl(device, VFIO_DEVICE_RESET);
```
