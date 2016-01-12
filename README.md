# el_usb_rt_read
Reads and logs temperature and humidity values from a EL-USB-RT or other similar devices.
No extra outdated libraries required, just a recent kernel version.

To compile:
gcc -o read_temp read_temp

To run:
./read_temp <hidraw file (e.g. /dev/hidraw1)> <logfile>
