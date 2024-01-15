# Roll your own network driver

- [link](https://hackaday.com/2018/01/01/34c3-roll-your-own-network-driver-in-four-simple-steps/)

## Not all applications run on top of HTTP

- Lots of network functionality is moving from specialized hardware to software:
  - Routers.
  - (Virtual) Switches.
  - Firewalls.
  - Middle-boxes.

```text
       |Firewall/Router/...Application|
                 ___||___
                |___OS___|
                 ___||___
                |_Driver_|
                    ||
   //===============||=================\\
 _||__       _||__       _||__       _||__
|_NIC_|     |_NIC_|     |_NIC_|     |_NIC_|
```

- Normal application:

```text
User space
              |Application|
                    || Socket API
|-------------------||----------------------|
Kernel space     ___||___
                |___OS___|
                 ___||___
                |_Driver_|
                    ||
   //===============||=================\\
 _||__       _||__       _||__       _||__
|_NIC_|     |_NIC_|     |_NIC_|     |_NIC_|
```

- BUT! What it really looks like:

```text
User space
              |Application|
                    ||
                    ||
              HERE BE DRAGONS!!!
                    ||
                    || Socket API
|-------------------||----------------------|
Kernel space     ___||___
                |___OS___|
                 ___||___
                |_Driver_|
                    ||
   //===============||=================\\
 _||__       _||__       _||__       _||__
|_NIC_|     |_NIC_|     |_NIC_|     |_NIC_|
```

## Performance

- Packets per second on a 10 GBit/s link: up to 14.88 Mpps
- Packets per second on a 100 GBit/s link: up to 148.8 Mpps
- Clock cycles per packet on a 3 GHz CPU with 14.88 Mpps: approximately 200 cycles.
- Typical performance target: approximately 5 to 10 Mpps per CPU core for simple forwarding.

## Performance: User space app

- Typical performance target: approximately 5 to 10 Mpps per CPU core for simple forwarding.
- 5 to 10 Mpps = 300 to 600 cycles per packet at 3 GHz
- Time to cross the user space boundary: very very long.
- Single-core forwarding performance with sockets: approximately 0.3 Mpps.
- Single-core forwarding performance with libpcap: approximately 1 Mpps.

## Move the application to the kernel

- We move our application to kernel space: module, driver, etc.
- But we have some new problems:
  - Cumbersome to develop.
  - Usual kernel restriction (e.g, C as programming language).
  - Application can (and will) crash the kernel.

## Performance: Kernel app

- Typically performance target: approximately 5 to 10 Mpps per CPU core for simple forwarding.
- 5 to 10 Mpps = 300 to 600 cycles per packet at 3 GHz
- Time to receive a packet in the Linux kernel: approximately 500 cycles.
- Time to send a packet in the Linux kernel: approximately 440 cycles.
- Time to allocate, initialize and free a `sk_buff` in the Linux kernel: approximately 400 cycles.

- Single-core forwarding performance with Open vSwitch: approximately 2 Mpps.
- Hottest topic in the linux kernel: XDP, which fixes some of these problems.

## Do more in user space?

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

- Using a shared memory for application-magic lib and map to kernel memory and driver.
- Some User space packet processing frameworks:
  - netmap.
  - PF_RING ZC.
  - pfq.

- But we have some problems:
  - Non-standard API, custom kernel module required.
  - Most frameworks require patched drivers.
  - Exclusive access to the NIC for one application.
  - No access to the usual kernel features.
    - Limited support for kernel integration in netmap.
  - Poor support for hardware offloading features of NICs.
  - Framework needs explicit support for each NIC, limited to a few NICs.

## Do even more in user space?

```text
User space
              |Application|------>|DMA     |
                    ||            |Memory  |
               | mmap'ed |------->|        |
               |   BAR0  |          ||
                    ||              ||
|-------------------||--------------||------|
Kernel space        ||              ||
                    ||              ||
                    ||              ||
   //===============||==============||==\\
 _||__       _||__       _||__         _||__
|_NIC_|     |_NIC_|     |_NIC_|       |_NIC_|
```

- No kernel driver running.
- Super fast.

- Some user space driver frameworks:
  - DPDK.
  - Snabb.

- We still have some problems:
  - Non-standard API.
  - Exclusive access to the NIC for one application.
  - Framework needs explicit support for each NIC model.
    - DPDK supports virtually all >= 10 Gbit/s NICs.
  - Limited support for interrupts.
    - Interrupts not considered useful at >= 0.1 Mpps.
  - No access to the usual kernel features.

## What has the kernel ever done for us?

- Lots of mature drivers.
- Protocol implementations that actually work (TCP, ...)
- Interrupts (NAPI is quite nice).
- Stable user space APIs.
- Access for multiple applications at the same time.
- Firewall, route, eBPF, XDP, ...
- And more.

## Are these frameworks fast?

- Typical performance target: approximately 5 to 10 Mpps per CPU core for simple forwarding.
- 5 to 10 Mpps = 300 to 600 cycles per packet at 3 GHz.

- Time to receive a packet in DPDK: approximately 50 cycles.
- Time to send a packet in DPDK: approximately 50 cycles.
- Other user space frameworks play in the same league.

- Single-core forwarding with Open vSwitch on DPDK: approximately 13 Mpps (2 Mpps without).
- Performance gains from: batching (typically 16 to 64 packets/batch), reduced memory overhead (no `sk_buff`).

## Can we build our own user space driver?

- Sure, but why?
  - For **FUN**.
  - To understand how NIC drivers work.
  - To understand how user space packet processing frameworks work.
    - Many people see these frameworks as magic black boxes.
    - DPDK drivers: >= 20K lines of code per driver.
  - How hard can it be?

## Hardware: Intel `ixgbe` family (10 Gbit/s)

- `ixgbe` family: 82599ES (aka X520), X540, X540, X550, Xeon D embedded NIC.
- Commonly found in servers or as on-board chips.
- Very good data-sheet publicly available.
- Almost no logic hidden behind black-box firmware.

## How to build a full user space driver in 4 simple steps

- 1. Upload kernel driver.
- 2. `mmap` the PCIe MMIO address space.
- 3. Figure out physical addresses for DMA.
- 4. Write the driver.

- 1. Find the device we want to use command: `lspci`

  ```text
  03:00.0 Ethernet controller: Intel Corporation 82599ES 10-Gigabit SFI/SFP + ...
  03:00.1 Ethernet controller: Intel Corporation 82599ES 10-Gigabit SFI/SFP + ...
  ```

- 2. Unload the kernel driver.

  ```bash
  echo 0000:03:00.1 > /sys/bus/pci/devices/0000:03:00.1/driver/unbind
  ```

- 3. mmap the PCIe configuration address space from user space.

  ```C
  int fd = open("/sys/bus/pci/devices/0000:03:00.0/resource0", O_RDWR);
  struct stat stat;

  fstat(fd, &stat);
  uint8_t *registers = (uint8_t *)mmap(NULL, stat.st_size, PROT_READ, PROT_WRITE, MAP_SHARED, fd, 0);
  ```

  - Device registers:

    ```text
    |Offset   |Abbreviation|Name                      |RW |
    |0x00000  |CTRL        |Device Control register   |RW |
    |0x00008  |STATUS      |Device status register    |RO |
    |0x00018  |CTRL_EXT    |Extended of DCR           |RW |
    |0x00020  |ESDP        |Extended SDP control      |RW |
    |0x00028  |I2CCTL      |I2C control               |RW |
    |0x00200  |LEDCTL      |LED control               |RW |
    |0x05078  |EXVET       |Extended VLAN Ether type  |RW |
    ```

  - For example, access registers: LEDs

    ```C
    #define LEDCTL 0x0200
    #define LED0_BLINK_OFFS 7

    uint32_t leds = *((volatile uint32_t *)(registers + LEDCTL));
    *((volatile uint32_t*))(registers + LEDCTL) = leds | (1 << LED0_BLINK_OFFS);
    ```

  - Memory-mapped IO: all memory accesses go directly to the NIC.
  - One of the very few valid uses `volatile` in C.

## How are packets handled?

- Packets are transferred via DMA (Direct Memory Access).
- DMA transfer is initiated by the NIC.
- Packets are transferred via queue interfaces (often called rings).
- NICs have multiple receive and transmit queues:
  - Modern NICs have 128 to 768 queues.
  - This is how NICs scale to multiple CPU cores.
  - Similar queues are also used in GPUs an NVMe disks.

## Rings/Queues

- Specific to `ixgbe`, but most NICs are similar.
- Rings are circular buffers filled with DMA descriptor.
  - DMA descriptors are 16 bytes: 8 byte physical pointer, 8 byte metadata
  - Translate virtual addresses to physical addressing using `/proc/self/pagemap`.
- Queue of DMA descriptors is accessed via DMA.
- Queue index pointers (head & tail) available via registers for synchronization.

- Ring memory layout:

```text
Physical memory:
   
   //========================\\
   ||     //==================||=====\\
   ||     ||     //===========||======||========\\
   ||     ||     ||           \/      \/         \/
|16byte|16byte|16byte|...|2kilobyte|2kilobyte|2kilobyte|...      |
 \__________________/     \_____________________________________/
          ||                                ||
   Descriptor Ring                      Memory Pool
```

## Receiving packets

- Tell NIC via MMIO the location and size of the ring.
- Fill DMA descriptors with pointers to allocated memory.

- NIC writes a packet via DMA and increments the head pointer.
- NIC also sets a status flag in the DMA descriptor once it's done.
  - Checking the status flag is way faster than reading the MMIO-mapped RDH register.

- Periodically poll the status flag.
- Process the packet.
- Reset the DMA descriptor, allocate a new packet or recycle.
- Adjust tail pointer (RDT) register.
