# A user space network driver

## Install dependency

```bash
./host_install.sh
```

## Up vagrant

```bash
sudo vagrant destroy nw_stack
sudo vagrant up nw_stack
```

## Run application

- Check PCI device info in the host:

```bash
sudo lspci -Qkxxxxnnv
```

- For example, in our case we have two PCI devices is two virtual network device.

```text
00:08.0 Ethernet controller [0200]: Red Hat, Inc. Virtio network device [1af4:1000]
        Subsystem: Red Hat, Inc. Device [1af4:0001]
        Flags: bus master, fast devsel, latency 64, IRQ 16
        I/O ports at d240 [size=32]
        Memory at f0806000 (32-bit, non-prefetchable) [size=8K]
        Capabilities: [40] Vendor Specific Information: VirtIO: CommonCfg
        Capabilities: [50] Vendor Specific Information: VirtIO: Notify
        Capabilities: [64] Vendor Specific Information: VirtIO: ISR
        Capabilities: [74] Vendor Specific Information: VirtIO: <unknown>
        Capabilities: [88] Vendor Specific Information: VirtIO: DeviceCfg
        Kernel driver in use: virtio-pci
00: f4 1a 00 10 07 00 10 00 00 00 00 02 00 40 00 00
10: 41 d2 00 00 00 00 00 00 00 60 80 f0 00 00 00 00
20: 00 00 00 00 00 00 00 00 00 00 00 00 f4 1a 01 00
30: 00 00 00 00 40 00 00 00 00 00 00 00 0b 01 00 00
40: 09 50 10 01 02 00 00 00 00 00 00 00 38 00 00 00
50: 09 64 14 02 02 00 00 00 38 00 00 00 32 00 00 00
60: 02 00 00 00 09 74 10 03 02 00 00 00 6c 00 00 00
70: 01 00 00 00 09 88 14 05 02 00 00 00 00 00 00 00
80: 04 00 00 00 01 00 00 00 09 00 10 04 02 00 00 00
90: 70 00 00 00 0a 00 00 00 00 00 00 00 00 00 00 00
a0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
b0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
c0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
d0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
e0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
f0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

00:09.0 Ethernet controller [0200]: Red Hat, Inc. Virtio network device [1af4:1000]
        Subsystem: Red Hat, Inc. Device [1af4:0001]
        Flags: bus master, fast devsel, latency 64, IRQ 17
        I/O ports at d260 [size=32]
        Memory at f0808000 (32-bit, non-prefetchable) [size=8K]
        Capabilities: [40] Vendor Specific Information: VirtIO: CommonCfg
        Capabilities: [50] Vendor Specific Information: VirtIO: Notify
        Capabilities: [64] Vendor Specific Information: VirtIO: ISR
        Capabilities: [74] Vendor Specific Information: VirtIO: <unknown>
        Capabilities: [88] Vendor Specific Information: VirtIO: DeviceCfg
        Kernel driver in use: virtio-pci
00: f4 1a 00 10 07 00 10 00 00 00 00 02 00 40 00 00
10: 61 d2 00 00 00 00 00 00 00 80 80 f0 00 00 00 00
20: 00 00 00 00 00 00 00 00 00 00 00 00 f4 1a 01 00
30: 00 00 00 00 40 00 00 00 00 00 00 00 0b 01 00 00
40: 09 50 10 01 02 00 00 00 00 00 00 00 38 00 00 00
50: 09 64 14 02 02 00 00 00 38 00 00 00 32 00 00 00
60: 02 00 00 00 09 74 10 03 02 00 00 00 6c 00 00 00
70: 01 00 00 00 09 88 14 05 02 00 00 00 00 00 00 00
80: 04 00 00 00 01 00 00 00 09 00 10 04 02 00 00 00
90: 70 00 00 00 0a 00 00 00 00 00 00 00 00 00 00 00
a0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
b0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
c0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
d0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
e0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
f0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
```

- Run application:

```bash
./pcap -o log -b 0000:00:08.0
```
