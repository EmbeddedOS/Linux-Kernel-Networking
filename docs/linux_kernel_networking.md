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

## 12. Wireless in Linux

- This chapter deals with the wireless stack in the Linux Kernel.
- Becoming familiar with the 802.11 MAC header is essential in order to understand the wireless subsystem implementation.

### 12.1. MAC 802.11 Subsystem

- It was only on about 2001, many of laptops were sold with wireless network interfaces. Today every laptop includes WiFi as standard equipment.
- It was important to the Linux community at that time to provide **Linux drivers to these wireless network interfaces** and to provide a **Linux wireless stack**, in order to stay competitive with other OSes.

- There are many differences between 802.11 and 802.3 wired Ethernet. Here are two major differences:
  - 1. Ethernet works with CSMA/CD, whereas 802.11 works with CSMA/CA.
    - CSMA/CA stands for carrier sense multiple access/**collision avoidance**, and CSMA/CD stands for carrier sense multiple access/**collision detection**.
    - With Ethernet, a station starts to transmit when the medium is idle; if a collision is detected during transmission, it stops, and a random back off period starts.
    - Wireless stations **cannot detect collisions** while transmitting, whereas wired station can. With CSMA/CA, the wireless station waits for a free medium and only then transmits the frame. In case of a collision, the station will not notice it, but because no acknowledgment frame should be sent for this packet, it is retransmitted after a timeout has elapsed if an acknowledgement is not received.

  - 2. **Wireless traffic is sensitive to interferences**. As a result, the 802.11 spec requires that every frame, except for broadcast and multicast, **be acknowledged when it is received**. Packets that are not acknowledged in time should be retransmitted.

### 12.2. The 802.11 MAC header

- Each MAC frame consists of a MAC header, a frame body of variable length, and an FC (Frame Check Sequence) of 32 bt CRC..

- IEEE 802.11 header:

```text
| Frame |Duration|Address|Address|Address|Sequence|Address|  QoS  |   HT  |
|Control|  /ID   |   1   |   2   |   3   |Control |   4   |Control|Control|
| 2bytes| 2 bytes|6 bytes|6 bytes|6 bytes|2 bytes |6 bytes|2 bytes|4 bytes|
```

- The header is represented in mac80211 by the `ieee80211_hdr` structure (`include/linux/ieee80211.h`):

```C
struct ieee80211_hdr {
    __le16 frame_control;
    __le16 duration_id;
    u8 addr1[6];
    u8 addr2[6];
    u8 addr3[6];
    __le16 seq_ctl;
    u8 addr4[6];
} __packed;
```

- In contrast to an Ethernet header (`struct ethhdr`), which contains only three fields (src MAC, des MAC, and Ethernet type), the 802.11 header contains up to six addresses and some other fields.
  - For a typical data frame, though, only three addresses are used.
  - With an ACK frame, only the receiver address is used.
  - Note that previous header format show only 4 addresses but when we working with Mesh network, a Mesh extension header with two additional addresses are used.

#### 12.2.1. The Frame Control

- Frame control fields:

```text
|<-B0-B1>|<B2-B3>|<B4-B7>|<-B8>|<-B9-->|<--B10-->|<B11>|<B12>|<B13>|<--B14-->|<-B15->|
|Protocol| Type  |Subtype|To DS|From DS|   More  |Retry|Power|More |Protected| +HTC/ |
| version|       |       |     |       |Fragments|     |Mgmt |Data |  Frame  | Order |
| 2 bits | 2 bits| 4 bits|1 bit| 1 bit |  1 bit  |1 bit|1 bit|1 bit|  1 bit  | 1 bit |
```

- The following is a description of the frame control members:
  - 1. **Protocol version**: version of the MAC 802.11 we use. Currently there is only one version of MAC, so this field is always 0.
  - 2. **Type**: There are three types of packets in 802.11:
    - 1. **Management Packets** are for management actions like: association, authentication, scanning, and more.
    - 2. **Control Packets** usually have some relevance to data packets; For example: a station wants to transmit first sends a control packet named RTS (**Request to Send**); if the medium is free, the destination station will send back a control packet named CTS (**Clear to Send**).
    - 3. **Data Packets** are the raw data packets. NULL packets are a special case of raw packets.
  - 3. **Subtype**: For all the aforementioned three types of packets (management, control, data), there is a sub-type field which identifies the character of the packet used. For example:
    - `0100` for the subtype field in a management frame denotes that the packet is a **Probe Request** management packet, which is used in a scan operation.
  - 4. **ToDS**: When the bit is set, it means the packet is for the distribution system.
  - 5. **FromDS**: When this bit is set, it means the packet is from the distribution system.
  - 6. **More flag**: When you use fragmentation, this bit is set to 1.
  - 7. **Retry**: When a packet is retransmitted, this bit is set to 1.
  - 8. **Pwr Mgmt**: When the power management bit is set, it means that the station will enter power save mode.
  - 9. **More Data**: When an AP sends packets that it buffered for a sleeping station, it sets the **More data** bit to 1 when the buffer is not empty.
  - 10. **Protected Frame**: This bit is set to 1 when the frame body is encrypted; only data frames and authentication frames can be encrypted.
  - 11. **Order**: With the MAC service called strict ordering. the order of frames is important.

### 12.3. The other 802.11 MAC header Members

- 1. **Duration ID**: holds values for the Network Allocation Vector (NAV) in microseconds, and it consists of 15 bits of the Duration/ID field.
- 2. **Sequence Control**: This is a 2-byte field specifying the sequence control. In 802.11, it is possible that a packet will be received more than once, most commonly when an acknowledgement is not received for some reason.
  - The sequence control field consists of a fragment number (4 bits) and a sequence number (12 bits).
- 3. **Address1 - Address4**: There are four addresses, but you don't always use all of them.
  - **Address1** is **Receive Address (RA)** and is used in all packets.
  - **Address2** is **Transmit Address (TA)**, and it exists in all packets except ACK and CTS packets.
  - **Address3** is used only for management and data packets.
  - **Address4** is used when **ToDS** and **FromDS** bits of the frame control are set.
- 4. **QoS Control**: The QoS control is only present in QoS data packets.
- 5. **HT Control Field**: HT (High Throughput).

### 12.4. Network Topologies

- There are two popular network topologies in 802.11 wireless networks.
  - 1. Infrastructure BSS (Basic Service Set) mode, which is the most popular: home or office, etc.
  - 2. IBSS (Ad Hoc) mode. Note that: IBSS is Independent BSS.

#### 12.4.1. Infrastructure BSS

- When working in this mode, there is a central device, called an Access Point (AP), and some client stations. Together they form a BSS (Basic Service Set). These client stations must first perform association and authentication against the AP to be able to transmit packets via the AP.

- On many occasions, client stations perform scanning prior to authentication and association, in order to get details about the AP.
- Association is exclusive:
  - 1. A client can be associated with only one AP in a given moment.
  - 2. When a client associates with an AP successfully, it gets an AID (Association ID), which is a unique number (to this BSS) in the range 1-2007.

- An AP is in fact a wireless network device with some hardware additions (Ethernet ports, LEDs, a button to reset to manufacture defaults, and more).

- A management daemon runs on the AP device. An example of such software is the `hostapd` daemon.
  - This software handles some of the management tasks of the MLME layer, such as authentication and association requests.
  - It achieves this by registering itself to receive the relevant management frames via `nl80211`.
  - The `hostapd` project is **an open source project** which enables several wireless network devices to operate as an AP.

- Clients can communicate with other clients (or to stations in a different network which is bridged to the AP) by sending packets to the AP, which are relayed by the AP to their final destination. To cover a large area, you can deploy multiple APs and connect them by wire.

#### 12.4.2. IBSS or Ad HoC mode

- IBSS network is often formed without preplanning, for only as long as the WLAN is needed.

### 12.5. Power Save Mode

- Apart from relaying packets, there is another important function for the AP: Buffering packets for client stations that enter power save mode.
  - Clients are usually battery-powered devices. From time to time, the wireless network interfaces enters power save mode.

#### 12.5.1. Entering Power Save Mode

- When a client station enters power save mode, it informs the AP about it by sending usually **a null data packet**.
- An AP that gets such a null packet starts keeping unicast packets which are destined to that station in a special buffer called `ps_tx_buf`; there is such a buffer for every station.
- **This buffer is in fact a linked list of packets**, and it can hold up to 128 packets (STA_MAX_TX_BUFFER) for each station. If the buffer is filled, it will start discarding the packets that were received first (FIFO).

- Apart from this, there is a single buffer called `bc_buf`, for multicast and broadcast packets (in the 802.11 stack, multicast packets should be received and processed by all the stations in the same BSS).
  - The `bc_buf` buffer can also hold up to 128 packets. When a wireless network interface is in power save mode, it cannot receive or send packets.

#### 12.5.2. Exiting Power Save Mode

- From time to time, an associated station is awakened by itself (by some timer); it then checks for special management packets, called **beacons**, which the AP sends periodically.
  - 1. Typically, an AP sends 10 beacons in a second, on most APs, this is configurable parameter.
  - 2. These beacons contain data in `information elements`; which constitute the data in management packet.
  - 3. The station that awoke checks a specific information element called TIM (Traffic Indication Map), by calling the `ieee80211_check_tim()` method (`include/linux/ieee80211.h`).
    - The TIM is an array of 2008 entries. Because the TIM size is 251 bytes (2008 bits), you are allowed to send a partial virtual bitmap, which is smaller in size. **If the entry in the TIM for that station is set, it means that the AP saved unicast packets for this station**, so the station should empty the buffer of packets that the AP kept for it.
- The station starts sending null packets to retrieve these buffered packets from the AP. Usually after the buffer has been emptied, the station goes to sleep.

#### 12.5.3. Handling the Multicast/broadcast buffer

- The AP buffers multicast and broadcast packets whenever at least one station is in sleep mode. **The AID for multicast/broadcast stations is 0**; so in such a case you set `TIM[0]` to true. The Delivery Team (DTIM), which is a special type of TIM, is sent not in every beacon, but once for a predefined number of beacon intervals.
  - After a DTIM is sent, the AP sends its buffered broadcast and multicast packets.
  - You retrieve packets from the multicast/broadcast buffer (bc_buf) by calling the `ieee80211_get_buffered_bc()` method.

### 12.6. The Management Layer (MLME)

- There are three components in the 802.11 management architecture:
  - 1. The Physical Layer Management Entity (PLME).
  - 2. The System Management Entity (SME).
  - 3. The MAC Layer Management Entity (MLME).

#### 12.6.1. Scanning

- There are two types of scanning: **passive scanning** and **active scanning**.
  - passive scanning means to listen passively for beacons, without transmitting any packets for scanning. The station moves from channel to channel, trying to receive beacons.
  - With active scanning, each station sends a Probe Request Packet; this is a management packet, with subtype Probe Request. The station moves from channel to channel, sending a Probe Request management packet on each channel (by calling the `ieee_80211_send_probe_req()` method).

- this is done by calling the `ieee80211_request_scan()` method.
- Changing channels is done via a call to the `ieee80211_hw_config()` method.

#### 12.6.2. Authentication

- Authentication is done by calling the `ieee80211_send_auth()` method (`/net/mac80211/util.c`). It sends a management frame with authentication sub-type.
- There are many authentication types;
- The original IEEE802.11 spec talked about only two forms: **open-system authentication** and **shared key authentication**.

- In shared key authentication, the station should authenticate using a Wired Equivalent Privacy (WEP) key.

#### 12.6.3. Association

- In order to associate, a station sends a management frame with association sub-type.
- Association is done by calling the `ieee80211_send_assoc()`.

#### 12.6.4. Reassociation

- When a station moves between APs within an ESS, it is said to be **roaming**. The roaming station sends a reassociation request to a new AP by sending a management frame with reassociation sub-type.

### 12.7. Mac80211 Implementation

- Mac80211 has an API for interfacing with the low level device drivers.

- A fundamental structure of mac80211 API is the `ieee80211_hw` structure (`/include/net/mac80211.h`); it represents hardware information.
  - The `priv` (pointer to a private area) pointer of `ieee80211_hw` is of an opaque type (`void *`).
  - Memory allocation and initialization for the `ieee80211_hw struct` is done by the `ieee80211_alloc_hw()` method.
  - Here are some methods related to the `ieee80211_hw struct`:
    - 1. `int ieee80211_register_hw(struct ieee80211_hw *hw)`: Called by wireless drivers for registering the specified `ieee80211_hw` object.
    - 2. `void ieee80211_unregister_hw(struct ieee80211_hw *hw)`: Unregister the specified 802.11 hardware device.
    - 3. `struct ieee80211_hw *ieee80211_alloc_hw(size_t priv_data_len, const struct ieee80211_ops *ops)`: Allocates an `ieee80211_hw` object and initializes it.
    - 4. `ieee80211_rx_irqsafe()`: This method is for receiving a packet. It is implemented in `/net/mac80211/rx.c` and called from low level wireless drivers.

- The `ieee80211_ops` object, which is passed to the `ieee80211_alloc_hw()` method, consists of pointers to callbacks to the driver. Not all of these callbacks must be implemented by the drivers. The following is a short description of these methods:
  - `tx()`: The transmit handler called for each transmitted packet.
  - `start()`: Activates the hardware device and is called before the first hardware device is enabled.
  - `stop()`: Turns off frame reception and usually turns off the hardware.
  - `add_interface()`: Called when a network device attached to the hardware is enabled.
  - `remove_interface()`: Informs a driver that the interface is going down.
  - `config()`: Handles configuration requests, such as hw channel configuration.
  - `configure_filter()`: Configures the device's Rx Filter.

- Linux Wireless Subsystem Architecture:

```text
 ___________________________________
|             User-space            |
|___________________________________|
 ______________   __________________
|      iw      | |  wireless-tools  |
|______________| |(iwconfig, iwlist)|
 ______________   __________________
|    nl80211   | |        wext      |
|______________| |__________________|
 ___________________________________
|              cfg80211             |
|___________________________________|
         _________________
        |  cfg80211_ops   |
        |                 |
        |    scan()       |
        |    auth()       |
        |    connect()    |
        |    ...          |
        |_________________|
 ___________________________________
|              MAC80211             |
|___________________________________|
         ____________________
        | ieee80211_ops      |
        |                    |
        | start()            |
        | stop()             |
        | add_interface()    |
        | remove_interface() |
        | ...                |
        |____________________|
 ___________________________________
|          Wireless Drivers         |
|___________________________________|
```

- Another important structure is th `sta_info struct` (`net/mac80211/sta_info.h`) which represents a station.
  - Among the members of this structure are various statistics counters, various flags, `debugfs` entries, the `ps_tx_buf` array for buffering unicast packets, and more.
  - Stations are organized in a hash table (`sta_hash`) and a list (`sta_list`). The important methods related to `sta_info`:
    - `int sta_info_insert(struct sta_info *sta)`: Adds a station.
    - `int sta_info_destroy_addr(struct ieee80211_sub_if_data *sdata, const u8 *addr)`: Removes a station.
    - `struct sta_info *sta_info_get(struct ieee80211_sub_if_data *sdata, const u8 *addr)`: Fetches a station; the address of the station (it's `bssid`) is passed as a parameter.

#### 12.7.1. Rx Path

- The `ieee80211_rx()` function (`net/mac80211/rx.c`) is the main receive handler. The status of the received packet (`ieee80211_rx_status`) is passed by the wireless driver to mac80211, embedded in the SKB control buffer (cb).

- In the `ieee80211_rx()` method, the `ieee80211_rx_monitor()` is invoked to remove th FCS (checksum) and remove a radiotap header (`struct ieee80211_radiotap_header`) which might have been added if the wireless interface is in monitor mode.

#### 12.7.2. Tx Path

- The `ieee80211_tx()` method is the main handler for transmission (`net/mac80211/tx.c`).
  - 1. First it invokes the `__ieee80211_tx_prepare()` method, which performs some checks and sets certain flags.
  - 2. Then it calls the `invoke_tx_handlers()` method, which calls, one by one, various transmit handlers.
  - 3. If a transmit handler finds that it should do nothing with the packet, it returns TX_CONTINUE and you proceed to the next handler. If it decides it should handle a certain packet, it returns TX_QUEUED, and if it decides it should drop the packet, it returns `TX_DROP`.
  - 4. The `invoke_tx_handlers()` return 0 upon success.

- A short look in the implementation of the `ieee80211_tx()` method:

```C
static bool ieee80211_tx(struct ieee80211_sub_if_data *sdata,
                         struct sk_buff *skb, bool txpending,
                         enum ieee80211_band band)
{
    struct ieee80211_local *local = sdata->local;
    struct ieee80211_tx_data tx;
    ieee80211_tx_result res_prepare;
    struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
    bool result = true;
    int led_len;

    // ...
    // Perform a sanity check, drop the SKB if its length is less than 10:
    if (unlikely(skb->len < 10)) {
        dev_kfree_skb(skb);
        return true;
    }

    // Initializes TX.
    led_len = skb->len;
    res_prepare = ieee80211_tx_prepare(sdata, &tx, skb);

    if (unlikely(res_prepare == TX_DROP)) {
        ieee80211_free_txskb(&local->hw, skb);
        return true;
    } else if (unlikely(res_prepare == TX_QUEUED)) {
        return true;
    }

    // ...
    // Invoke the Tx Handler; if everything is fine, continue with invoking the `__ieee80211_tx()` method:
    if (!invoke_tx_handlers(&tx))
        result = __ieee80211_tx(local, &tx.skbs, led_len, tx.sta, txpending);

    return true;
}
```

#### 12.7.3. Fragmentation

- Fragmentation in 802.11 is done only for unicast packets. Each station is assigned a fragmentation threshold size (in bytes). Packets that are bigger than the this threshold should be fragmented.
- **You can lower the number of collisions by reducing the fragmentation threshold size, making the packets smaller**.

- You can inspect the fragmentation threshold of a station by running `iwconfig` or by inspecting the corresponding `debugfs` entry.
- You can set the fragmentation threshold with the `iwconfig` command; thus, for example, you can set the fragmentation threshold to 512 bytes by:

```bash
iwconfig wlan0 frag 512
```

- Each fragment is acknowledged. The more fragment field in the fragment header is set to 1 if there are more fragments. Each fragment has a fragment number. Reassembling of the fragments on the receiver is done according to the fragment numbers.

- Fragmentation in the transmitter side is done by the `ieee80211_tx_h_fragment()` method (`net/mac80211/tx.c`).
- Reassembly on the receiver side is done by the `ieee80211_rx_h_defragment()` method (`net/mac80211/rx.c`).

#### 12.7.4. Mac80211 debugfs

- `debugfs` is a technique that enables exporting debugging information to user-space.
- It creates entries under the `sysfs` filesystem.
- `debugfs` is a virtual filesystem devoted to debugging information. For mac80211, handling mac80211 `debugfs` is mostly in `net/mac80211/debugfs.c`. After mounting `debugfs`, various mac80211 statistics and information entries can be inspected. Mounting `debugfs` is performed like this:

```bash
mount -t debugfs none_debugs /sys/kernel/debug
```

- For example, let's say your `phy` is `phy0`; the following is a discussion about some of the entries under `/sys/kernel/debug/ieee80211/phy0`.
  - `total_ps_buffered`: This is the total number of packets (unicast and multicast/broadcasts) which the AP buffered for the station.
  - Under `/sys/kernel/debug/ieee80211/phy0/statistics`, you have various statistical information--for example:
    - `frame_duplicate_count`: denotes the number of duplicate frames.
    - `transmitted_frame_count`: denotes the number of transmitted packets.
    - `retry_count`: denotes number of retransmissions.
    - `fragmentation_threshold`: The size of the fragmentation threshold, in bytes.
  - Under `/sys/kernel/debug/ieee80211/phy0/netdev:wlan0` you have some entries that give information about the interface.

#### 12.7.5. Wireless Mode
