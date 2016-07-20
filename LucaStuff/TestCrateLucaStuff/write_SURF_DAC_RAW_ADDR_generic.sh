#!/bin/sh

echo "Writing in DAC at address " $2  " value: " $3
sudo ./sio_generic $1 w $2 $3
sudo ./sio_generic $1 w 6 4

