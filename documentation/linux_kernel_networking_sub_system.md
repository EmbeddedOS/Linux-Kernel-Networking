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

## 3. `net/core`, `net/sched`, and `net/socket.c`

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
