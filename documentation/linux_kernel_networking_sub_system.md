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

## 3. `net/core`, `net/sched`, and `net/socket.c`
