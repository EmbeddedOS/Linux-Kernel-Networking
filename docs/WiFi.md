# WiFi

- WiFi is a family of wireless network protocols based on the IEEE 802.11 family of standards, which are commonly used Local Area Network of devices and internet access, allowing nearby digital devices to exchange data by radio waves.

- These are most widely used computer networks, used globally in Home and small office networks to link devices and to provide Internet Access Wireless Routers and Wireless Access Points in public places such as coffee shops, hotels, libraries, and airports to provide visitors.

- WiFi uses multiple parts of the IEEE 802 protocol family and is designed to work seamlessly with its wired sibling, **Ethernet**.

- Compatible devices can network through wireless Access Points with each other as well as with wired devices and the Internet.

## 1. Etymology and terminology

- To connect to a WiFi-LAN, a computer must be equipped with a **wireless network interface controller**. The combination of a computer and an interface controller is called a **station**. Stations are identified by one or more **MAC addresses**.

A **Service Set** is the set of all the devices associated with a particular WiFi Network. Devices in a service set need not be on same wavebands or channels. Each service set has an associated identifier, a 32 byte **Service Set IDentifier (SSID)**.

## 2. Operational principles

- WiFi stations communicate by sending each other data packets, blocks of data individually sent and delivered over radio. This is done by the **modulation and demodulation** of carrier waves.

- As with other IEEE 802 LANs, stations come programmed with a globally unique 48-bit MAC address. The MAC addresses are used to specify both the destination and the source of each data packet. A network interface normally does not accept packets addressed to other WiFi stations.

- Channels are used **half duplex** and can be **time-shared** by multiple networks.
  - When communication happens on the same channel, any information sent by one computer is locally received by all, even if that information is intended for just one destination.
  - The network interface card interrupts the CPU only when applicable packets are received: The card ignores information not addressed to it.
  - The use of the same channel also means that the data bandwidth is shared, such that, for example, available data bandwidth to each device is halved when two stations are actively transmitting.

- A scheme known as **carrier-sense multiple access with collision avoidance (CSMA/CA)** governs the way stations share channels. With CSMA, CA stations attempt to avoid collisions by beginning transmission only after the channel is sensed to be idle, but then transmit their packet data in its entirely.
  - CSMA/CA cannot completely prevent collisions.
  - A collisions happens when a station receives signals from multiple stations on a channel at the same time.
    - This corrupts the transmitted data and can require stations to re-transmit. The lost data and re-transmission reduces throughput, in some cases severely.

### 2.1. Communication Stack

- WiFi is part of IEEE 802 protocol family. The data is organized into 802.11 frames that are very similar to Ethernet frames at the **data link layer**, but with extra address fields. MAC addresses are used as network addresses for routing over the LAN.

```text
|<--2-->|<---2-->|<--6-->|<--6-->|<--6-->|<-0--2->|<--6-->|<-0-2->|<-0-4->|<0-2312>|<-4->|
| Frame |Duration|Address|Address|Address|Sequence|Address|  QoS  |   HT  | Frame  | FCS |
|Control|  /ID   |   1   |   2   |   3   |Control |   4   |Control|Control|  Body  |     |
```

- WiFi's MAC and physical layer (PHY) specifications are defined by IEEE 802.11 for modulating and receiving one or more carrier waves to transmit the data in the infrared and 2.4, 3.6, 5, 6, or 60 GHz frequency bands.

- For inter-networking purposes, WiFi is usually layered as a **link layer** (equivalent to the physical and data link layers of the OSI model) below the internet layer. This means that nodes have an associated Internet Address and, with suitable connectivity, this allows full Internet access.
