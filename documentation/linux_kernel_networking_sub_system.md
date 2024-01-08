# Linux Kernel Networking Sub-system

## 1. Introduction Kernel Architecture

```text
Application
------------System Call Interface-------------------------------------------
Kernel      ||          ||          ||              ||                ||
            ||          ||          ||              ||                || 
        ____\/____    __\/__     ___\/______     ___\/__             _\/___
       | Memory   | |Process|   |File system|   |Network|           |Device|
       |Management| |Manager|     __||____       ___||____          |Driver|
            ||          ||       |FS types|     |Protocols|           ||
      |(VM)Memory|   ___||____   ___||_______    ___||_________       ||  
      |  Manager |  |Scheduler| |Block driver|  |Network driver|      ||
         ___||__________||__________||______________||________________||__
        |___  __________  _______Device Drivers_____  ________________  __|       
------------||----------||----------||--------------||----------------||----
Hardware    ||          ||          ||              ||                ||
            \/          \/          \/              \/                \/
           RAM          CPU     HDD/Storage        NIC              Devices
```

- If we have knowledge about subsystem, we can optimize for specific kernel, etc.

## 2. Kernel source code

- View kernel on web: [link](https://elixir.bootlin.com/linux/v6.7-rc8/source)

- `/mm`: memory management sub module.
- `/fs`: file system.
- `drivers`:
- `/net`: networking subsystem.
  - `/core`: important files like `dev.c` that is responsible for device routine, device APIs, etc.
  - `/ipv4`: Handle Ipv4 protocol and upper layers like: ARP, Ping, tunnel, net-filter, ICMP, TCP, UDP.
  - `/ipv6`:
  - `/sched`: Queue algorithm, handle packet, various queue types.

## 3. `/net/core`, `net/sched`, and `net/socket.c`

```text
                        Socket APIs:
                           recvmsg()
                           write()
                           sendmsg()
                              /\
------------------User space--||--------------------------------------------
                     _________\/__
                    |net/socket.c | -> sock_recvmsg(), sock_sendmsg()
                    /\
            |-------||------Network Protocol Stack-------------------------|
            |       ||                                                     |
     _______|_______\/__________________________                           |
    |_______|___net/core________________________| -> dev.c, dst.c, skbuff.c|
     /\     |        /\          /\         /\                             |
     ||     |        ||          ||         ||                             |
     \/     |        \/          \/         \/                             |
|net sched| |    |IPv4 stack| |Ipv6 stack| |etc.|                          |
            |                                                              |
```

- `/net/core/dev.c`
  - `void list_netdevice(struct net_device *dev);` -> Device list insertion.
  - `static void unlist_netdevice(struct net_device *dev, bool lock);` -> Device list removal.
  - `struct net_device *dev_get_by_name(struct net *net, const char *name);` -> Find a device by its name (in the list).
  - `int dev_open(struct net_device *dev, struct netlink_ext_ack *extack);` -> Prepare an interface for use. Takes a device from down to up state. -> `ifconfig eth0 up` command using this function.
  - `void dev_close(struct net_device *dev);` -> Shutdown an interface.

- `/net/sched/`: Queue algorithm, handle packet, various queue types.
- `/net/socket.c`: socket APIs.

## 4. `/net/core/dev.c` Packets transmit, receive APIs

- `/net/core/dev.c`:
  - Tx routines: [packet_transmission.png](../resources/packet_transmission.png)
    - IPv4 stack -> `dev_queue_xmit_sk()` -> `__dev_queue_xmit()` -> `dev_hard_start_xmit()` -> Device (ring buffer).
    - `dev_queue_xmit_nit()` -> Network TAPs.
    - `__dev_xmit_skb()`

  - Rx routines: [packet_reception.png](../resources/packet_reception.png)
    - Device (ring buffer) -> `netif_receive_skb()` -> `__netif_receive_skb()` -> `__netif_receive_skb_core()` -> IPv4 stack.

- This file have three parts:
  - Device management APIs: `list_netdevice()`, `unlist_netdevice()`, etc.
  - Packet transmitter routine.
  - Packet receiver routine.

- `__dev_queue_xmit()` is very important API that transmit a buffer: Queue a buffer for transmission to a network device.

## 5. `/net/core/skbuff.c` APIs

- `sk_buff` structure is very important structure that represent for network packets.
  - `sk_buff` objects are maintain in kernel with linked list.

- Some important APIs:
  - `build_skb()`: build a network buffer.
  - `kfree_skb()`: Release anything attached to the buffer.
  - `skb_clone()`: duplicate an sk_buff.
  - `skb_put()`: add data to a buffer.
  - `skb_push()`: add data to the start of a buffer.
  - `skb_trim()`: remove end from a buffer.

## 6. Interrupt context

- `in_interrupt()`: return nonzero if in interrupt context, and zero if in process context. This includes either executing an interrupt handler or a bottom half handler.
  - `in_irq()`: Returns nonzero if currently executing an interrupt handler and zero otherwise.
  - `in_softirq()`

- We need to check it in kernel network development because we need to determine what context we are developing.

- More often, you want to check whether you are in process context. That is, you want to ensure you are not in interrupt context. This is often the case because code wants to do something that can only be done from process context, such as sleep. If in_interrupt() returns zero, the kernel is in process context.

## 7.  tcp_parse_options() API to parse TCP options in Linux kernel networking sub-system
