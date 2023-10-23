# Kernel bypass techniques for high-speed network packet processing

- [link](https://www.youtube.com/watch?v=MpjlWt7fvrw)

## Typical packet flow

```text
    |       TX      |                   |       RX      |
    |Application    |                   |Application    |
    |Transport(L4)  |                   |Transport(L4)  |
    |Network(L3)    |                   |Network(L3)    |
    |Data link(L2)  |                   |Data link(L2)  |
    |NIC Driver     |                   |NIC Driver     |
    |NIC hardware   |                   |NIC hardware   |
                \\                       //
                 \\>>>>>>>>>>>>>>>>>>>>>//
```

- The packet go through TX application to RX application via multi layers.

## What does a packet contain?

```text
|Ethernet Header|IP header|TCP header|payload|FCS|
```

- The main content of Ethernet header: `|Dest MAC|Src MAC|Type/Length|`
- IP header: `|...|length|...|IP type|header checksum|src IP| dst IP|`
- TCP header: `|Src Port|Dst Port|...|checksum|...|`
- FCS: Ethernet Frame Check sequence.
- NIC: Network interface card.

## RX path: packet arrives at the destination NIC

```text
|Application|                           User space |
|--------------------------------------------------|
                                      kernel space |

       __________________________
      |________NIC_Driver________|
        /\       ||            /\        |pack buff|
        ||       \/            ||        |  ...    |
        ||     //<<<\\       //<<<\\---> |pack buff|
        ||    || TX  ||     ||  RX ||    |pack buff|
        ||     \\>>>//       \\>>>//
        ||       ||             /\
|-------||-------||-------------||-----------------|
        ||       ||             ||        Hardware |
        ||       ||             ||
        ||       \\===>|NIC|===>// HW RX queue
        ||             ||
        ||------------//
      Hardware Interrupt
```

- NIC receives the packet:
  - Match destination MAC address.
  - Verify Ethernet check sum (FCS).
- Packets accepted at the NIC:
  - DMA the packet to RX ring buffer.
  - NIC triggers an interrupt to inform packets is received and available in memory.

- TX/RX rings:
  - Circular queue.
  - Shared between NIC and NIC driver.
  - Content: length + packet buffer pointer.

- The flow receive like this:
  - 1. Some packet come to NIC, and NIC push it to Hardware RX queue.
  - 2. NIC pop a packet from Hardware RX queue and push to RX ring (by DMA packet to the ring).
    - NIC driver actually allocated memory for all items in rings.
    - The NIC just copy to the pointer of item.
  - 3. NIC triggers an interrupt to inform packets is received and available in memory.
  - 4. NIC driver pop the RX ring and move to layer 3 handler.

## Interrupt processing in the Linux Kernel

- Interrupts are trigger by NIC causes the CPU to pause or disrupt the process in execution.

- Interrupt processing in the linux kernel is divided to two parts:
  - Top-half: Minimal processing.
  - Bottom-half: Rest of interrupt processing.

### Top-half interrupt processing

- CPU interrupts the process in execution.
- Switch from user space to kernel space.
- Top-half interrupt processing:
  - 1. Lookup IDT (Interrupt Descriptor Table).
  - 2. Call corresponding ISR (Interrupt Service Routine)
    - Acknowledge the interrupt.
    - **Schedule bottom-half processing**.
  - 3. Switch back to user space.

### Bottom-half processing

- 1. CPU initiates the bottom-half when it is free (soft-irq).
- 2. CPU context switches to the kernel space.
- 3. NIC Driver dynamically allocates an `sk-buff` (socket buffer structure) for the received packet.
  - `sk-buff` in-memory data structure that contains packet metadata.
    - Pointers to packet headers and payload.
    - More packet related information.

        ```text
        |Application|                           User space |
        |--------------------------------------------------|
                                            kernel space |

            __________________________
            |________NIC_Driver________|                        `sk-buff`
                /\       ||            /\        |pack buff|<------/
                ||       \/            ||        |  ...    |
                ||     //<<<\\       //<<<\\---> |pack buff|
                ||    || TX  ||     ||  RX ||    |pack buff|
                ||     \\>>>//       \\>>>//
                ||       ||             /\
        |-------||-------||-------------||-----------------|
                ||       ||             ||        Hardware |
                ||       ||             ||
                ||       \\===>|NIC|===>// HW RX queue
                ||             ||
                ||------------//
            Hardware Interrupt
        ```

    - NOTE: The kernel maintain single copy of a packet with in the packet buffers. If multiple packets are required, for example tools like `wireshark`, `tcpdump` require individual copies then the escape of `sk-buff` is cloned and every clone point to the same packet in memory.

- 4. NIC Driver update `sk-buff` with packet metadata.
- 5. NIC Driver remove the Ethernet header.
- 6. NIC Driver pass `sk-buff` to the network stack.
- 7. NIC Driver call Layer 3 protocol handler.

- NIC Driver do it step 3 -> 7 for all packets in the RX buffer.

## Layer 3, Layer 4 processing

- Common processing:
  - Matching destination IP/socket.
  - Verify checksum.
  - Remove header.

- L3 specific processing:
  - 1. Route Lookup.
  - 2. Combine fragmented packets.
  - 3. Call L4 protocol handler.

- L4 specific processing:
  - 1. Handle TCP state machine.
  - 2. Enqueue to socket read queue.
  - 3. Signal the socket to notify to application.

```text
|Application read()|                                             User space |
|-----/\--------------------------------------------------------------------|
      ||                                                       kernel space |
      ||
      ||=================================================\\
                                                          ||
                                        |       |       |       |
                                        | Write |       | Read  |
                                        | Queue |       | Queue |
                                        |       |       |       |
     __________________________                         |-------|
    |________NIC_Driver________|                        |sk-buff|
        /\       ||            /\        |pack buff|<------/
        ||       \/            ||        |  ...    |
        ||     //<<<\\       //<<<\\---> |pack buff|
        ||    || TX  ||     ||  RX ||    |pack buff|
        ||     \\>>>//       \\>>>//
        ||       ||             /\
|-------||-------||-------------||------------------------------------------|
        ||       ||             ||                                 Hardware |
        ||       ||             ||
        ||       \\===>|NIC|===>// HW RX queue
        ||             ||
        ||------------//
    Hardware Interrupt
```

- Actually the write queue and read queue holds pointers to the packet.
- On socket read(): user space to kernel space:
  - 1. Dequeue packet from socket receive queue (kernel space).
  - 2. Copy packet to **application buffer** (user space).
  - 3. Release sk-buff.
  - 4. Return back to the application.

## Transmit path od an application packet

```text
|Application write()|                                            User space |
|---||-----------||-----------------------------------------------------------|
    ||           ||                                              kernel space |
    ||           ||=Buffer================================\\
    ||___________                                         ||
    |System_calls|                                        \/
           ||                           |       |       |       |
           ||   _____________           | Read  |       | Write |
           ||  |Network_stack|          | Queue |       | Queue |
           ||                           |       |       |       |
     ______\/__________________                         |-------|
    |________NIC_Driver________|                        |sk-buff|
        /\       ||            /\        |pack buff|<------/
        ||       \/            ||        |  ...    |
        ||     //<<<\\       //<<<\\---> |pack buff|
        ||    || RX  ||     ||  TX ||    |pack buff|
        ||     \\>>>//       \\>>>//
        ||       /\             ||
|-------||-------||-------------||------------------------------------------|
        ||       ||             ||                                 Hardware |
        ||       ||             ||
        ||       \\<===|NIC|<===// HW TX queue
        ||             ||
        ||------------//
    Hardware Interrupt
```

- On socket write: user space to kernel space.
  - 1. Writes the packet to the kernel buffer (user buffer is copied to not hold pointer).
  - 2. Call socket's send function (e.g. sendmsg())
  - 3. socket send invoke L4 processing. For example TCP.
  - 4. TCP dynamically allocates `sk-buff` structure for the packet and enqueues it to the socket Write Queue.
    - Build headers.
    - Add header to packet buffer.
    - Update `sk-buff`.
  - 5. TCP call L3 protocol handler.
  - 6. L3 fragment, if needed.
  - 7. Call L2 protocol handler.
  - 8. L2 enqueue packet to **queue discipline** (`qdisc`).
    - Hold packets in a queue.
    - Apply scheduling policies (e.g. FIFO, priority.)

- `qdisc`:
  - Dequeue `sk-buff` (if NIC has free buffers).
  - Post process `sk-buff`.
    - Calculate IP/TCP checksum.
    - ...(tasks that hardware cannot do).
  - Call NIC driver's send function.
- NIC Driver:
  - if hardware transmit queue full.
    - stop `qdisc` queue.
  - otherwise:
    - map packet data for DMA.
    - Tells NIC to send the packet.
- NIC:
  - Calculates ethernet frame checksum.
  - Sends packet to the wire.
  - Sends an interrupt `packet is sent` (kernel space to user space).
  - Driver frees the `sk-buff`; starts the `qdisc` queue.

## Summary packet processing overheads in the kernel

- Too many context switches.
  - Pollutes CPU cache.
- Per-packet interrupt overhead.
- Dynamic copy between kernel and user space.
- Shared data structured.
