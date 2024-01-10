#!/bin/bash

setup_vagrant() {
    sudo apt install qemu qemu-kvm libvirt-clients libvirt-daemon-system \
    virtinst bridge-utils vagrant -y

    sudo systemctl enable libvirtd
    sudo systemctl start libvirtd
    }

apt-get install -y cmake build-essential pciutils vim