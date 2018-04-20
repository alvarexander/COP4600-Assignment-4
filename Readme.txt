The input device requires to be installed before the output device after the makefile builds them.
sudo insmod inputdevice.ko
sudo insmod outputdevice.ko

The output device must be removed before the input device.
sudo rmmod outputdevice
sudo rmmod inputdevice