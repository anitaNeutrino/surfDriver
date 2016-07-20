#!/bin/sh

case  $2  in
                0VT)
                    let addr=0x80
                    ;;
                0HT)
                    let addr=0x86
                    ;;
                0VM)
                    let addr=0x81
                    ;;
                0HM)
                    let addr=0x87
                    ;;
                0VB)
                    let addr=0x82
                    ;;
                0HB)
                    let addr=0x88
                    ;;
                1VT)
                    let addr=0x83
                    ;;
                1HT)
                    let addr=0x89
                    ;;
                1VM)
                    let addr=0x84
                    ;;
                1HM)
                    let addr=0x8A
                    ;;
                1VB)
                    let addr=0x85
                    ;;
                1HB)
                    let addr=0x8B
                    ;;
              *)
                    echo "Error: antenna " $1 " does not exist"
                    exit
          esac
echo "Writing in DAC at address " $addr  " value: " $3
sudo ./sio_generic $1 w $addr $3
sudo ./sio_generic $1 w 6 4

