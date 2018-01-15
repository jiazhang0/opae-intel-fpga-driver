#!/bin/sh
depmod -A
modprobe spi-nor-mod
modprobe altera-asmip2
modprobe fpga-mgr-mod
modprobe intel-fpga-pci
modprobe intel-fpga-afu
modprobe intel-fpga-fme

if [ -x /sbin/udevadm ]; then
      /sbin/udevadm control --reload-rules
      /sbin/udevadm trigger
fi
