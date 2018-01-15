#!/bin/sh
if [ -n "`lsmod | grep fpga`" ]; then
  rmmod intel-fpga-fme
  rmmod intel-fpga
  rmmod fpga-mgr-mod
  rmmod altera-asmip2
  rmmod spi-nor-mod
  rmmod intel-fpga-pci
fi

if [ -d $DESTDIR/etc/udev/rules.d ]; then
  cp $DESTDIR/usr/src/intel-fpga-0.13.0/40-intel-fpga.rules $DESTDIR/etc/udev/rules.d
fi
