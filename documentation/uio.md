# The User-space I/O HOW TO

## Preface

- For many types of devices, creating a Linux Kernel Driver is overkill. All that is really needed is some way to handle an interrupt and provide access to the memory space of the device. The logic of controlling the device does not necessary have to be within the kernel, as the device does not need to take advantage of any of other resources that the kernel provides. One such common class of devices that are like this are for industrial I/O cards.

- To address this situation, the user-space I/O system (UIO) was designed. For typical industrial I/O cards, Only a very small kernel module is needed. **The main part of the driver will run in user space**. This simplifies development and reduces the risk of serious bugs within a kernel module.

- Please note that UIO is not an universal driver interface. Devices that are already handled well by other kernel sub-systems (like networking or serial or USB) are no candidates for an UIO driver. Hardware that ideally suited for an UIO driver fulfills all of the following:
  - The device has memory that can be mapped. The device can be controlled completely by writing to this memory.
  - The device usually generates interrupts.
  - The device does not fit into one of the standard kernel sub-systems.

## About UIO

- If you use UIO for your card's driver, here's what you get:
  - 1. **Only one small kernel module to write and maintain**.
  - 2. **Develop the main part of your driver in user space, with all the tools and libraries you are used to.**
  - 3. **Bugs in your driver won't crash the kernel.**
  - 4. **Updates of your driver can take without recompiling the kernel.**

## How UIO works

- Each UIO device is accessed through a device file and severals sysfs attributes files. The device file will be called `/dev/uio0` for the first device, and `/dev/uio1`, `/dev/uio2` and so on for subsequent devices.

- `/dev/uioX` is used to access the address space of card. Just use `mmap()` function to access registers or RAM locations of your card.

- A interrupts are handled by reading from `/dev/uioX`. A blocking `read()` function from `/dev/uioX` will return as soon as an interrupt occurs.
  - You can also use `select()` function on `/dev/uioX` to wait for an interrupt. The integer value read from `/dev/uioX` represents the total interrupt count. You can use this number to figure out if you missed some interrupts.

- To handle interrupts properly, your customer kernel module can provide its own interrupt handler. It will automatically be called by the built-in handler.

- For cards that don't generate interrupts but need to be polled, there is the possibility to set up a timer that triggers the interrupt handler at configurable time intervals. This interrupt simulation is done by calling `uio_event_notify()` from the timer's event handler.

- Each driver provides attributes that are used to read or write variables. These attributes are accessible through sysfs files. A custom kernel driver module can add its own attributes to the device owned by the uio driver, but not added to the UIO device itself at this time. This might change in the future if it would be found to be useful.

- The following standard attributes are provided by the UIO framework:
  - `name`: The name of your device. It is recommended to use the name of your kernel module for this.
  - `version`: A version string defined by your driver. This allows the user space part of your driver to deal with different versions of the kernel module.
  - `event`: The total number of interrupts handled by the driver since the last time the device node was read.

- These attributes appear under the `/sys/class/uio/uioX` directory. Please note that this directory might be a symlink, and not a real directory. Any user-space code that accesses it must be able to handle this.

- Each UIO device can make one or more memory regions available for memory mapping. This is necessary because some industrial I/O cards require access to more than one PCI memory region in a driver.

- Each mapping has its own directory in sysfs, the first mapping appears as `/sys/class/uio/uioX/maps/map0/`. Subsequent mappings create directories `map1`, `map2`, and so on. These directories will only appear of the size of the mapping is not 0.

- Each `mapX` directory contains four read-only files that show attributes of the memory:
  - `name`: A string identifier for this mapping. This is optional, the string can be empty. Drivers can set this to make it easier for user-space to find the correct mapping.
  - `addr`: the address of memory that can be mapped.
  - `size`: The size, in bytes, of the memory pointed to by `addr`.
  - `offset`: the offset, in bytes, that has to be added to the pointer returned by `mmap()` to get the actual device memory. This is important if the device's memory is not page aligned. Remember that pointers returned by `mmap()` are always page aligned, so it is good style to always add this offset.

- From user-space, the different mappings are distinguished by adjusting the `offset` parameter of the `mmap()` call. To map the memory of mapping N, you have to use N times the page size as your offset:

    ```C
    offset = N * getpagesize();
    ```

- Sometimes there is hardware with memory-like regions that can not be mapped with the technique described here, but there are still ways to access them from user-space. The most common example are x86 io-ports. On x86 systems, user-space can access these io-ports using `ioperm()`, `iopl()`, `inb()`, `outb()`, and similar functions.

## Writing your own kernel module

### struct `uio_info`

- This structure tells the framework the details of your driver, some of the members are required, others are optional.
  - `const char *name`: Required. The name of your driver as it will appear in sysfs. I recommend using the name of your module for this.
  - `const char *version`: Required. This string appears in `/sys/class/uio/uioX/version`.
  - `struct uio_mem mem[MAX_UIO_MAPS]`: Required if you have memory that can be mapped with `mmap()`. For each mapping you need to fill one of the `uio_mem` structures.
  - `struct uio_port [MAX_UIO_PORTS_REGIONS ]`: Required if you want to pass information about io-ports to user space. For each port region you need to fill one of the `uio_port` structures.
  - `long irq`: Required. If your hardware generates an interrupt, it's your modules task to determine the irq number during initialization. If you don't have a hardware generated interrupt but want to trigger the interrupt handler in some other way, set `irq` to `UIO_IRQ_CUSTOM`. If you had no interrupt at all, you could set `irq` to `UIO_IRQ_NONE`, through this rarely makes sense.
  - `unsigned long irq_flags`: Required if you've set `irq` to a hardware interrupt number. The flags given here will be used in the call to `request_irq()`.
  - `int (*mmap)(struct uio_info *info, struct vm_area_struct *vma)`: Optional. If you need a special `mmap()` function, you can set it here. If this pointer is not NULL, your `mmap()` will be called instead of the built-in one.
  - `int (*open)(struct uio_info *info, struct inode *inode)`: Optional. You might want to have your own `open()`, e.g. to enable interrupts only when your device is actually used.
  - `int (*release)(struct uio_info *info, struct inode *inode)`: Optional. If you define your own `open()`, you will probably also want a custom `release()` function.
  - `int (*irqcontrol)(struct uio_info *info, s32 irq_on)`: Optional. If you need to be able to enable or disable interrupts from user-space by writing to `/dev/uioX`, you can implement this function, The parameter `irq_on()` will be 0 to disable interrupts and 1 to enable them.

- Usually, your device will have one or more memory regions that can be mapped to user space. For each region, you have yo set up a `struct uio_mem` in the `mem[]` array. Here's a description of the fields of `struct uio_mem`:
  - `const char *name`: Optional. Set this to help identify the memory region, it will show up in the corresponding sysfs node.
  - `int memtype`: Required if the mapping is used. Set this to `UIO_MEM_PHYS` if you have physical memory on your card to mapped. Use `UIO_MEM_LOGICAL` for logical memory (e.g. allocated with `kmalloc()`). There's also `UIO_MEM_VIRTUAL` for virtual memory.
  - `phys_addr_t addr`: Required if the mapping is used. Fill in the address of your memory block. This address is the one that appears in sysfs.
  - `resource_size_t size`: Fill in the size of the memory block that `addr` points to. If `size` is zero, the mapping is considered unused. Note that you must initialize `size` with zero for all unused mappings.
  - `void *internal_addr`: If you have to access this memory region from within your kernel module, you will want to map it internally by using something like `ioremap()`. Addresses returned by this function cannot be mapped to user space, so you must not store it in `addr`.

- NOTE: Please do not touch the map element of struct uio_mem!

### Adding an interrupt handler

- What you need to do in your interrupt handler depends on your hardware and on how you want to handle it. You should try to keep the amount of code in your kernel interrupt handler low. If your hardware requires no action that you have to perform after each interrupt, then your handler can be empty.

- If, on the other hand, your hardware needs some action to be performed after each interrupt, then you must do it in your kernel module. Note that you cannot rely on the user-space part of your driver. Your user-space program can terminate at any time, possibly leaving your hardware in a state where proper interrupt handling is still required.

## Writing a driver in user-space

- Once you have a working kernel module for your hardware, you can write the user-space part of your driver. You don't need any special libraries, your driver can be written in any reasonable language, you can use floating point numbers and so on. In short, you can use all the tool and libraries you'd normally use for writing a user-space application.

### Getting information about your UIO device

- Information about all UIO devices is available in sysfs. The first thing you should do in your driver is check `name` and `version` to make sure your talking to the right device and that its kernel driver has the version you expect.
- You should also make sure that the memory mapping you need exists and has the size you expect.
- There is a tool called `lsuio` that lists UIO devices and their attributes. It is available: [User](http://www.osadl.org/projects/downloads/UIO/user/).

- With `lsuio` you can quickly check if your kernel module is loaded and which attributes it exports.

- The source code of `lsuio` can serve as an example for getting information about an UIO device. The file `uio_helper.c` contains a lot of functions you could use in your user-space driver code.

### `mmap()` device memory

- After you make sure you've got the right device with the memory mappings you need, all you have to do is to call `mmap()` to map the device's memory to user-space.

- The parameter `offset` of the `mmap()` call has a special meaning for UIO devices: It is used to select which mapping of your device you want to map. To map the memory of mapping N, you have to use N times the page size as your offset:

    ```C
    offset = N * getpagesize();
    ```

- N starts from zero, so if you've got only one memory range to map, set `offset=0`. A drawback of this technique is that memory is always mapped beginning with its start address.

### Waiting for interrupts

- After you successfully mapped your devices memory, you can access it like an ordinary array. Usually, you will perform some initialization. After that, your hardware starts working and will generate an interrupt as soon as it's finished, has some data available, or needs your attention because an error occurred.

- `/dev/uioX` is a read-only file. A `read()` will always block until an interrupt occurs.

- You can also use `select()` on `/dev/uioX`.

## Generic PCI UIO driver

- The generic driver is a kernel module named `uio_pci_generic`. It can work with any device compliant to PCI 2.3 and any compliant PCI Express device. Using this, you only need to write the user-space driver, removing the need to write a hardware-specific kernel module.

### Making the driver recognize the device

- Since the driver does not declare any devices ids, it will not get loaded automatically and will not automatically bind to any devices, you must load it and allocate id to the driver yourself.
