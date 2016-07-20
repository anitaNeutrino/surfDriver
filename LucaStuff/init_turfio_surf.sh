cd ~/turfio_test_driver/
sudo /sbin/insmod turfioDriver.ko device=0a:0d
sudo mknod /dev/turfio c 247 0
cd ../surfDriver/
sudo /sbin/insmod surfDriver.ko exclude=0a:0d
sudo chmod 666 /dev/turfio
sudo chmod 666 /dev/09\:09 
dmesg
