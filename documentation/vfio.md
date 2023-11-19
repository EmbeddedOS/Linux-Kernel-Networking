# VFIO - "Virtual Function I/O"

- Many modern system now provide DMa and interrupt remapping facilities to help ensure I/O devices behave within the boundaries they've been allotted. The VFIO driver is an IOMMU/device agnostic framework for exposing direct device access to user-space, in a secure, IOMMU protected environment. In other words, this allows safe, non-privileged, user-space drivers.

- Why do we want that?
  - Virtual machines often make use of direct device access ("device assignment") when configured for the highest possible I/O performance. From a device and host perspective, this simply turns the VM into a user-space driver, with the benefits of significantly reduced latency bandwidth, and direct use of bare-metal device drivers.

- Some applications, particularly in the high performance computing field, also benefit from-overhead, direct device access from user-space. Example include network adapters (often non-TCP/IP based) and compute accelerators. Prior to VFIO, these drivers has to either go through the full development cycle to become proper up-stream driver, be maintained out of tree, or make use of the UIO framework, which has no notion of IOMMU protection, limited interrupt support, and requires root privileges to access things like PCI configuration space.

- The VFIO driver framework, intends to unify these, replacing both the KVM PCI specific device assignment code as well as provide a more secure, more feature-ful user-space driver environment than UIO.

## Groups, Device, and IOMMUs

- **Devices are the main target of any I/O driver. Devices typically create a programming interface made up of I/O access, interrupts, and DMA.**

- Without going into the details of each of these, DMA is by far the most critical aspect for maintaining a secure environment as allowing a device read-write access to system memory imposes the greatest risk to the overall system integrity.

- To help mitigate this risk, many modern IOMMUs now incorporate isolation properties into what was, in many cases, an interface only meant for translation (ie. solving the addressing problems of devices with limited address spaces). With this devices can now be isolated from each other and from arbitrary memory access, this allowing things like secure direct assignment of devices into VMs.
