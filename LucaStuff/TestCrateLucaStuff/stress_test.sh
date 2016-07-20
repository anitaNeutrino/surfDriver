#!/bin/sh
for ((i=0;i<100000;i++))
do echo -n $i  " "
./tio_noprint w 8 1 
./read_surf_ready 09:09
./sio_generic_noprint 09:09 w 6 2 
./sio_generic_noprint 09:09 w 6 2 
./tio_noprint w 0xc 2 
./tio_noprint w 0xc 2 
./tio_printdec r 11 
./sio_generic_printdec 09:09 r 5
echo
done

