# Linux Kernel Networking

- This document deals with the implementation of the Linux Kernel Networking stack and theory behind it.
- Linux Began as an Intel x86-based OS but has been ported to a very wide range of processors, including ARM, PowerPC, MIPS, SPARC, and more. The Android OS, based upon the Linux kernel, is common today in tablets and smartphones, and seems likely to gain popularity in the future in smart TVs.
- The Linux kernel networking stack is a very important subsystem of the Linux kernel. it is quite difficult to find a Linux-based system, whether it is a desktop, a server, a mobile device or any other embedded device, that does not use any kind of networking.

- **The Linux Network Stack**:
  - There are seven logical networking layers according to the Open System Interconnection OSI model. The lowest is the physical layer, the lowest layer is the physical layer, which is the hardware, and the highest layer is the application layer, where user space software processes are running. Let's describe these seven layers:
    - 1. *The physical layer*: Handles electrical signals and the low level details.
    - 2. *The data link layer*: Handles data transfer between endpoints. The most common data link layer is ethernet. The Linux Ethernet network device drivers reside in this layer.
    - 3. *The network layer*: Handles packet forwarding and host addressing. The most common network layers of the Linux Kernel Networking subsystem: IPv4 or IPv6.
    - 4. *The protocol layer/transport layer*: layer handles data sending between nodes. The TCP and UDP protocols are the best-known protocols.
    - 5. *The session layer*: Handles sessions between endpoints.
    - 6. *The presentation layer*: Handles delivery and formatting.
    - 7. *The application layer*: Provides network services to end-user applications.
  - Three layers that the Linux Kernel Networking stack handles.
    - 1. L2.
    - 2. L3 (IPv4, IPv6)
    - 3. L4 (TCP/UDP, ...)
  - The L2, L3, and L4 layers in this figure correspond to the data link layer, the network layer, and the transport layer in the seven-layer model, respectively. The essence of the Linux Kernel stack is **passing incoming packets from Layer 2 (network device driver) to Layer 3 (Network layer) and then to Layer 4 if they are for local delivery or back to Layer 2 for transmission when the packets should be forwarded**. Outgoing packets that were locally generated are passed from Layer 4 to Layer 3 and then to Layer 2 for actually transmission by the network device driver. Along this way there are many stages, and many things can happen. For example:
    - The packet can be changed due to protocol rules (for example, due to an IPsec rule or to a NAT rule).
    - The packet can be discarded.
    - The packet can cause an error message to be sent.
    - The packet can be fragmented.
    - The packet can be de-fragmented.
    - A checksum should be calculated for the packet.
  - The kernel does not handle any layer above layer 4; those layers (the session, presentation, and application layers) are handled solely by user space application.

- **The Network Device**:
  - The lowest layer 2 is the linker layer. The network device drivers reside in this layer. The `net_device` structure represents a network device, and some of the concepts that are related to it. Parameters of the device - like the size of MTU, which is typically 1500 bytes for Ethernet devices - determine wether a packet should be fragmented. The `net_device` is a very large structure, consisting of device parameters like these:
    - The IRQ number of the device.
    - The MTU of the device.
    - The MAC address of the device.
    - The name of the device (like `eth0` or `eth1`).
    - The flags of the device (for example, whether it is up or down).
    - A list of multicast addresses associated with the device.
    - The `promiscuity` counter.
    - The features that the device supports (like GSO or GRO offloading).
    - An object of network device callbacks (`net_device_ops` object), which consists of function pointers, such as for opening and stopping a device, starting to transmit, changing, the MTU of the network device, and more.
    - And object of `ethtool` callbacks, which supports getting information about the device by running the command-line `ethtool` utility.
    - The number of Tx and Rx queues, when the device supports multi-queues.
    - The timestamp of the last transmit of packet on this device.
    - The timestamp of the last reception of a packet on this device.
    - The following is the definition of some of the members of the `net_device` structure (include/linux/netdevice.h):

        ```C
        struct net_device {
        unsigned int irq; /* device IRQ number */
        ...
        const struct net_device_ops *netdev_ops;
        ...
        unsigned int mtu;
        ...
        unsigned int promiscuity;
        ...
        unsigned char *dev_addr;
        ...
        };
        ```

  - **Receiving and Transmitting Packets**:
    - The main tasks of the network device driver are these:
      - 1. To receive packets destined to the localhost and to pass them to the network layer (Layer 3), and from there to the transport layer (Layer 4).
      - 2. To transmit outgoing packets generated on the localhost and sent outside, or to forward packets that were received on the localhost.

    - For each packet, incoming or outgoing, a lookup in the `routing` subsystem is performed. **The decision about whether a packet should be forwarded and on which interface it should be sent is done based on the result of the lookup in the subsystem**.
      - The lookup in the `routing` subsystem is not the only factor that determines the traversal of a packet in the network stack. For example, there are five points in the network stack where callbacks of the `netfilter` subsystem can be registered.
        - The first netfilter hook point of a received packet is `NF_INET_PRE_ROUNTING`, before a routing lookup was performed. When a packet is handled by such a callback, which is invoked by a macro named `NF_HOOK()`, it will continue its traversal in the networking stack according to the result of this callback (also called `verdict`). For example:
          - 1. If the `verdict` is `NF_DROP`, the packet will be discarded,
          - 2. and if the `verdict` is `NF_ACCEPT` the packet will continue its traversal as usual.
        - The Netfilter hooks callbacks are registered by the `nf_register_hook()` method or by the `nf_register_hooks()` method, and you encounter these invocations, for example, in various netfilter kernel modules. The kernel netfilter subsystem is the infrastructure for the well-known `iptables` user space package.
    - Besides the `netfilter` hooks, the packet traversal can be influenced by the `IPsec` subsystem - for example, when matches a configured `IPsec` policy. `IPsec` provides a network layer security solution, and it uses the ESP and the AH protocols. `IPsec` is mandatory according to IPv6 specification and optional in IPv4.
      - `IPsec` has two modes of operation:
        - 1. transport mode.
        - 2. tunnel mode.
      - It is used as a basis for many **Virtual Private Network (VPN)** solutions, though there are also `non-IPsec` VPN solutions.
    - Still other factors can influence the traversal of the packet--for example, the value of the `ttl` field in the IPv4 header of a packet being forwarded. This `ttl` **is decremented by 1 each forwarding device**. when i reaches 0, the packet is discarded, and an `ICMPv4` message of *Time Exceeded* with *TTL Count Exceeded* code is sent back. This is done to avoid an endless journey of a forwarded packet because of some error.
      - Moreover, each time a packet is forwarded successfully and the `ttl` is decremented by 1, the checksum of the IPv4 header should be recalculated, as its value depends on the Ipv4 header, and the `ttl` is one of the IPv4 header members.

  - **The Socket Buffer**:
    - The `sk_buff` structure represents a packet. SKB stands for *socket buffer*. A packet can be generated by a local socket in the local machine, which was created by a user space application; the packet can be sent outside or to another socket in the same machine. A packet can also be created by a kernel socket; And you can receive a physical frame from a network device (Layer 2) and attach it to an `sk_buff` and pass it on to Layer 3. When the packet destination is your local machine, it will continue to Layer 4. If the packet is not for your machine, it will be forwarded according to your routine tables rules, if your machine support forwarding. If the packet is damaged for any reason, it will be dropped. The `sk_buff` is a very large structure;
    - Here is the (partial) definition of the sk_buff structure (include/linux/skbuff.h):

        ```C
        struct sk_buff {
        ...
        struct sock *sk;
        struct net_device *dev;
        ...
        __u8 pkt_type:3,
        ...
        __be16 protocol;
        ...
        sk_buff_data_t tail;
        sk_buff_data_t end;
        unsigned char *head,
        *data;
        sk_buff_data_t transport_header;
        sk_buff_data_t network_header;
        sk_buff_data_t mac_header;
        ...
        };
        ```

    - When a packet is received on the wire, an SKB is allocated by the network device driver, typically by calling the `net_dev_alloc_skb()`. There are cases along the packet traversal where a packet can be discard, and this is done by calling `kfree_skb()` or `dev_kfree_skb()`, both of which get as a single parameter a pointer to an SKB. Some members of the SKB are determines in the linker (Layer 2).
      - For example, the `pkt_type` will be set to `PACKET_BROADCAST`; and if this address is the address of the localhost, the `pkt_type` will be set to `PACKET_HOST`.
      - Most Ethernet network drivers call the `eth_type_trans()` method in their Rx path. The `eth_type_trans()` method also sets the protocol field of the SKB according to the `ethertype` of the Ethernet header, by calling the `skb_pull_inline()` method. The reason for this is that the `skb->data` should point to the header of the layer in which it currently resides. When the packet was in Layer 2, in the network device driver Rx path, `skb->data` pointed to the Layer (Ethernet) header; now that the packet is going to be moved to Layer 3, immediately after the call to the `eth_type_trans()` method, `skb->data` should point to the network (Layer 3) header, which starts immediately after the Ethernet header. An IPv4 packet:

        ```text
        | Ethernet header |     IPv4 header     | UDP header | Payload |
        |    14 bytes     | 20 bytes - 60 bytes |  8 bytes   |         |
        ```

    - The SKB includes the packet headers (Layer 2, Layer 3 and Layer 4 headers) and the packet payload. In the packet traversal in the network stack, a header can be added or removed. For example, for an IPv4 packet generated locally by a socket and transmitted outside, the network layer (IPv4) add an IPv4 header to the SKB.

    - Each SKB has a `dev` member, which is an instance of the `net_device` structure. For incoming packets, it is the incoming network device, and for outgoing packets it is the outgoing network device.
    - Each transmitted SKB has a `sock` object associated to it `sk`. If the packet os a forwarded packet, then `sk` is NULL, because it was not generated on the localhost.

    - Each received packet should be handled by a matching network layer protocol handler. For example, an IPv4 packet should be handled by the `ip_rcv()` method and an IPv6 packet should be handled by the `ipv6_rcv()` method.

    - In IPv4 there is a problem of limited address space, as an Ipv4 address is only 32 bit. Organizations use NAT to provide local addresses to their hosts, but the IPv4 address space still diminishes over the years.
      - One of the main reasons for developing the IPv6 protocol was that its address space is huge compared to the IPv4 address space, because the IPv6 address length is 128 bits.
      - But the Ipv6 protocol is not only about a larger address space. The Ipv6 address protocol includes many changes and additions as a result of the experience gained over the years with the Ipv4 protocol.
        - For example, the Ipv6 header has a fixed length of 40 bytes as opposed to the IPv4 header, which is variable in length.
        - Processing IP options in IPv4 is complex and quite heavy in terms of performance.
        - Another notable change is with the ICMP protocol; in Ipv4 it was used only for error reporting and for informative messages. In IPv6, the ICMP protocol is used for many other purposes: Neighbour Discovery (ND), for multicast listener discovery (MLD), and more.

    - Packets generated by the localhost are created by Layer 4 sockets, for example, by TCP sockets or by UDP sockets. They are created by a user-space application with the Socket API. There are two main types of sockets: **datagram** sockets and **stream** sockets.

    - Every layer 2 network interface has an L2 address that identifies it. In the case of Ethernet, this is a 48-bit address, **the MAC address which is assigned for each Ethernet network interface**, provided by the manufacturer, and said to be unique (could be change in user-space with `ifconfig` or `ip`). Each ethernet packet starts with an Ethernet header, which is 14 bytes long. It consists of the Ethernet type (2 bytes), the source MAC address (6 bytes), and the destination MAC address (6 bytes). The Ethernet type value is 0x0800, for example, for IPv4, or 0x86DD for IPv6.
      - For each outgoing packet, an Ethernet header should be built. When a user-space socket sends a packet, it specifies it destination address (it can be an IPv4 or an Ipv6 address). This is not enough to build the packet, as the destination MAC address should be known.

    - The network stack should communicate with the user-space for tasks such as adding or deleting routes, configuring neighboring tables, setting IPec policies and states, and more.

## 2. Netlink sockets

- The netlink socket interface appeared first in the 2.2 Linux Kernel as AF_NETLINK socket. It was created as a more flexible alternative to the awkward IOCTL communication method between user space processes and the kernel. The IOCTL handlers cannot send asynchronous messages to user-space from the kernel, whereas netlink sockets can. In order to use IOCTL, there is another of complexity: you need to define IOCTL numbers. The operation model of netlink is quite simple: You open and register a netlink socket in user space using socket API, and this netlink socket handles bidirectional communication with a kernel netlink socket, usually sending messages to configure various system settings and getting responses back from the kernel.

### 2.1. The Netlink Family

- The netlink protocol is a socket-based Inter Process Communication (IPC) mechanism,  based on RFC 3549, **Linux Netlink as an IP Services Protocol**. It provides a bidirectional communication channel between user space and kernel or among some parts of the kernel itself. Netlink is an extension of the standard socket implementation. The netlink protocol implementation resides mostly under `net/netlink`, where you will find the following four files:
  - `af_netlink.c`
  - `af_netlink.h`
  - `genetlink.c`
  - `diag.c`

- Apart from them, there are a few header files. In fact, the `af_netlink` module is the most commonly used; it provides the netlink kernel socket API, whereas the `genetlink` module provides a new generic netlink API with which is should be easier to create netlink messages. The `diag` monitoring interface module (`diag.c`) provides an API to dump and to get information about the netlink sockets.

- Netlink has some advantages over other ways of communication between user space and the kernel.
  - For example, there is no need for polling when working with netlink sockets. A user space application opens a socket and then calls `recvmsg()`, and enters a blocking state if no messages are sent from the kernel; see, for example, the `rtln_listen()` method of the `iproute2` package (`lib/libnetlink.c`).
  - Another advantage is that the kernel can be the initiator of sending asynchronous messages to user space, without any need for the user space to trigger any action.
  - Yet another advantage is that netlink sockets sockets support multicast transmission.

- You create netlink socket from user space with `socket()` system call. The netlink sockets can be `SOCK_RAW` sockets or `SOCK_DGRAM` sockets.

- Netlink sockets can be created in the kernel or in user space; kernel netlink sockets are created by the `netlink_kernel_create()` method; and user space netlink sockets are created by the `socket()` system call. Creating a netlink socket from user space or from the kernel creates a `netlink_sock` object. When the socket is created from user space, it is handled by the `netlink_create()` method. When the socket is created in the kernel, it is handled `__netlink_kernel_create()`; this method sets the `NETLINK_KERNEL_SOCKET` flag. Eventually both methods call `__netlink_create()` to allocate a socket in the common way (by calling the `sk_alloc()` method) and initialize it.

- Creating a netlink socket in the kernel and in user space:

```text
            |   socket(AF_NETLINK, SOCK_RAW| SOCK_CLOEXEC, NETLINK_ROUTE)      |
            |               User space netlink socket                          |
User space  |____________________________________  ____________________________|
-------------------------------------------------||-----------------------------
Kernel                                           ||
                                                 \/
                                            | netlink_create()                 |
                                                 ||
                                                 \/
            | netlink_kernel_create() |====>| __netlink_create()               |
            | Kernel Netlink socket   |     |   sk_alloc()                     |
                                            |   sock_init_data(sock, sk);      |
                                            |   ...                            |
```

- You can create a netlink socket from user space in a very similar way to ordinary BSD-style sockets, like this, for example: `socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)`. Then you should create a `sockaddr_nl` object (instance of the netlink socket address structure), initialize it, and use the standard BSD sockets API (such as `bind()`, `sendmsg()`, `recvmsg()`, and so on). The `sockaddr_nl` structure represents a netlink socket address in user space or in the kernel.

#### 2.1.1. Netlink Sockets Libraries
