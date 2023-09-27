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

    - When a packet is received on the wire, an SKB is allocated by the network device driver, typically by calling the `net_dev_alloc_skb()`. There are cases along the packet traversal where a packet can be discard, and this is done by calling `kfree_skb()` or `dev_kfree_skb()`, both of which get as a single parameter a pointer to an SKB. Some members of the SKB are determines
