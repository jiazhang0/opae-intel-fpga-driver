#!/bin/sh
if [ -f $DESTDIR/etc/udev/rules.d/40-intel-fpga.rules ]; then
  rm -f $DESTDIR/etc/udev/rules.d/40-${MODULE_NAME}.rules
fi

if [ -n "`lsmod | grep fpga`" ]; then
  rmmod intel-fpga-fme
  rmmod intel-fpga-afu
  rmmod intel-fpga-pci
  rmmod fpga-mgr-mod
  rmmod altera-asmip2
  rmmod spi-nor-mod
fi
