#!/bin/bash
# 
# setup_hugepages.sh 
#
# 1. What is huge page in Linux?
# Huge pages are helpful in virtual memory management in the Linux system. As
# the name suggests, they help is managing huge size pages in memory in addition
# to standard 4KB page size. You can define as huge as 1GB page size using huge
# pages.
#
# During system boot, you reserve your memory portion with huge pages for your
# application. This memory portion i.e these memory occupied by huge pages is
# `never swapped out of memory`. It will stick there until you change your
# configuration. This increases application performance to a great extent like
# Oracle database with pretty large memory requirements.
#
# 2. Why use huge page?
# In virtual memory management, the kernel maintains a table in which it has a
# mapping of the virtual memory address to a physical address. For every page
# transaction, the kernel needs to load related mapping. If you have small size
# pages then you need to load more numbers of pages resulting kernel to load
# more mapping tables. This decreases performance.
#
# Using huge pages means you need fewer pages. This decreases the number of
# mapping tables to load by the kernel to a great extent. This increases your
# kernel-level performance which ultimately benefits your application.
# 
# In short, by enabling huge pages, the system has fewer page tables to deal
# with and hence less overhead to access/maintain them!
#
# Why use huge page in our driver?
# Huge-pages must be enabled for running our driver for high performance. Huge
# page support is required to reserve large amount size of pages, 2MB or 1GB per
# page, to less TBL (Translation Lookaside Buffers) and to reduce cache miss.
# Less TBL means that it redece the time for translating virtual address to
# physical.
#
# How to configure?
# HugeTLBFS: For the creation of shared or private mappings, Linux provides a
# RAM-based filesystem called "hugetlbfs."
# More detailed: https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt

mkdir -p /mnt/huge
(mount | grep /mnt/huge) > /dev/null || mount -t hugetlbfs hugetlbfs /mnt/huge
for i in {0..7}
do
	if [[ -e "/sys/devices/system/node/node$i" ]]
	then
		echo 512 > /sys/devices/system/node/node$i/hugepages/hugepages-2048kB/nr_hugepages
	fi
done
