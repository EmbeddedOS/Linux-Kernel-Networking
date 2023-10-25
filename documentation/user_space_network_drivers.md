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
