#!/bin/sh

sudo rmmod kmlab
make
sudo insmod kmlab.ko
./userapp 5 &
 ./userapp 35 &
sleep 5
echo "== read 1 ==" && cat /proc/kmlab/status
sleep 5
echo "== read 2 ==" && cat /proc/kmlab/status
sleep 15
echo "== read 3 ==" && cat /proc/kmlab/status