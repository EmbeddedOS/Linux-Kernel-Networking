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

```bash
./pcap -o log -b 0000:00:08.0
```
